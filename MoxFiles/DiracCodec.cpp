/*
 *  DiracCodec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 7/28/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/DiracCodec.h>

#include <MoxFiles/Thread.h>

namespace MoxFiles
{

static unsigned int
PixelLayoutDepth(PixelType type)
{
	// see SMPTE 377M E.2.46
	switch(type)
	{
		case MoxFiles::UINT8:
			return 8;
		
		case MoxFiles::UINT10:
			return 10;
			
		case MoxFiles::UINT12:
			return 12;
			
		case MoxFiles::UINT16:
			return 16;
			
		case MoxFiles::UINT16A:
			assert(false); // not storing Adobe 16-bit
			return 16;
		
		case MoxFiles::UINT32:
			assert(false);
			return 32;
		
		case MoxFiles::HALF:
			assert(false);
			return 253;
		
		case MoxFiles::FLOAT:
			assert(false);
			return 254;
	}
	
	throw MoxMxf::ArgExc("Unknown pixel type");
}

static unsigned int
PixelLayoutBits(PixelType type)
{
	const unsigned int depth = PixelLayoutDepth(type);
	
	return (depth == 254 ? 32 :
			depth == 253 ? 16 :
			depth);
}


static PixelType
PixelTypeFromBits(unsigned int bit_depth)
{
	switch(bit_depth)
	{
		case 8:
			return MoxFiles::UINT8;
		
		case 10:
			return MoxFiles::UINT10;
		
		case 12:
			return MoxFiles::UINT12;
		
		case 16:
			return MoxFiles::UINT16;
		
		case 32:
			assert(false);
			return MoxFiles::UINT32;
		
		case 253:
			assert(false); // no standard for this yet
			return MoxFiles::HALF;
			
		case 254:
			return MoxFiles::FLOAT;
	}
	
	throw MoxMxf::ArgExc("Unknown pixel type");
}


DiracCodec::DiracCodec(const Header &header, const ChannelList &channels) :
	VideoCodec(header, channels),
	_descriptor(NULL),
	_encoder(NULL),
	_decoder(NULL)
{	
	
	schro_init();
	
	_encoder = schro_encoder_new();
	
	if(_encoder == NULL)
		throw MoxMxf::NullExc("Error creating encoder");
	
	
	// see struct SchroEncoderSettings in schroencoder.c
	schro_encoder_setting_set_double(_encoder, "gop_structure", SCHRO_ENCODER_GOP_INTRA_ONLY);
	
	if( isLossless(header) )
	{
		schro_encoder_setting_set_double(_encoder, "rate_control", SCHRO_ENCODER_RATE_CONTROL_LOSSLESS);
	}
	else
	{
		const int quality = getQuality(header);
		
		const double schro_quality = (double)quality / 10.0;
	
		schro_encoder_setting_set_double(_encoder, "rate_control", SCHRO_ENCODER_RATE_CONTROL_CONSTANT_QUALITY);
		schro_encoder_setting_set_double(_encoder, "quality", schro_quality); // 0.0 - 10.0
	}
	
	
	const bool isRGB = (channels.findChannel("R") != NULL);
	
	int depth = 8;
	
	if(isRGB)
	{
		assert(channels.findChannel("Y") == NULL);
		
		MoxMxf::RGBADescriptor *rgb_descriptor = new MoxMxf::RGBADescriptor(header.frameRate(), header.width(), header.height(), MoxMxf::VideoDescriptor::VideoCodecDiracRGB);
		
		const MoxFiles::Channel &chan = channels["R"];
		
		depth = PixelLayoutDepth(chan.type);
		
		MoxMxf::RGBADescriptor::RGBALayout layout;
		
		layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('G', depth));
		layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('B', depth));
		layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('R', depth));
		
		rgb_descriptor->setPixelLayout(layout);
		
		_descriptor = rgb_descriptor;
	}
	else
	{
		_descriptor = new MoxMxf::CDCIDescriptor(header.frameRate(), header.width(), header.height(), MoxMxf::VideoDescriptor::VideoCodecDiracCDCI);
		
		assert(false); // not actually handling this right now
	}
	
	setWindows(*_descriptor, header);


	SchroVideoFormat *format = schro_encoder_get_video_format(_encoder); // this allocates a new version with a copy of the internal struct
	
	if(format)
	{
		//schro_video_format_set_std_video_format(format, SCHRO_VIDEO_FORMAT_HD1080P_60);
		//schro_video_format_set_std_frame_rate(format, 1);
		
		format->width = header.width();
		format->height = header.height();
		
		if(isRGB)
		{
			format->chroma_format = SCHRO_CHROMA_444;
			
			const int max_val = (1L << depth) - 1;
			
			format->luma_offset = 0;
			format->luma_excursion = max_val;
			format->chroma_offset = 0;
			format->chroma_excursion = max_val;
		}
		else
		{
			format->chroma_format = SCHRO_CHROMA_444;
		
			const SchroSignalRange range = (depth == 12 ? SCHRO_SIGNAL_RANGE_12BIT_VIDEO :
											depth == 10 ? SCHRO_SIGNAL_RANGE_10BIT_VIDEO :
											SCHRO_SIGNAL_RANGE_8BIT_VIDEO);
		
			schro_video_format_set_std_signal_range(format, range);
		
			format->colour_primaries = SCHRO_COLOUR_PRIMARY_HDTV;
			format->colour_matrix = SCHRO_COLOUR_MATRIX_HDTV; // SCHRO_COLOUR_MATRIX_REVERSIBLE
			format->transfer_function = SCHRO_TRANSFER_CHAR_TV_GAMMA; // SCHRO_TRANSFER_CHAR_LINEAR

			assert(false); // not handling right now
		}
		
		
		format->interlaced = false;
		//format->top_field_first = true;
		
		format->frame_rate_numerator = header.frameRate().Numerator;
		format->frame_rate_denominator = header.frameRate().Denominator;
		
		format->aspect_ratio_numerator = header.pixelAspectRatio().Numerator;
		format->aspect_ratio_denominator = header.pixelAspectRatio().Denominator;
		
		schro_encoder_set_video_format(_encoder, format);
		
		free(format);
	}
	else
		assert(false);
	
	schro_debug_set_level(SCHRO_LEVEL_WARNING);
	
	schro_encoder_start(_encoder);
}


DiracCodec::DiracCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(NULL),
	_encoder(NULL),
	_decoder(NULL)
{
	if(descriptor.getVideoCodec() == MoxMxf::VideoDescriptor::VideoCodecDiracRGB)
	{
		const MoxMxf::RGBADescriptor &rgb_descriptor = dynamic_cast<const MoxMxf::RGBADescriptor &>(descriptor);
		
		const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = rgb_descriptor.getPixelLayout();
		
		const int num_channels = pixelLayout.size();
		
		for(int i = 0; i < num_channels; i++)
		{
			assert(pixelLayout[i].code == 'R' || pixelLayout[i].code == 'G' || pixelLayout[i].code == 'B' || pixelLayout[i].code == 'A');
			
			if(pixelLayout[i].code != 'F') // padding
			{
				const std::string chan_name(1, pixelLayout[i].code);
				
				const PixelType pixelType = PixelTypeFromBits(pixelLayout[i].depth);
				
				channels.insert(chan_name, Channel(pixelType));
			}
		}
		
		_descriptor = new MoxMxf::RGBADescriptor(rgb_descriptor);
	}
	else if(descriptor.getVideoCodec() == MoxMxf::VideoDescriptor::VideoCodecDiracCDCI)
	{
		const MoxMxf::CDCIDescriptor &cdci_descriptor = dynamic_cast<const MoxMxf::CDCIDescriptor &>(descriptor);
	
		_descriptor = new MoxMxf::CDCIDescriptor(cdci_descriptor);
		
		assert(false); // not handling this yet
	}
	else
		throw MoxMxf::ArgExc("Bad descriptor input");

	schro_init();
	
	_decoder = schro_decoder_new();
	
	if(_decoder == NULL)
		throw MoxMxf::NullExc("Error creating decoder"); 
	
	//schro_decoder_set_skip_ratio(_decoder, 1.0);
}


DiracCodec::~DiracCodec()
{
	if(_encoder != NULL)
		schro_encoder_free(_encoder);
		
	if(_decoder != NULL)
		schro_decoder_free(_decoder);
	
	delete _descriptor;
}


class Convert2Signed : public Task
{
  public:
	Convert2Signed(TaskGroup *group, const Slice &slice, const Box2i &dw, int y);
	virtual ~Convert2Signed() {}
	
	virtual void execute();

  private:
	template <typename STYPE, typename UTYPE, int DIFF>
	void ConvertRow();

  private:
	const Slice &_slice;
	const Box2i &_dw;
	const int _y;
};


Convert2Signed::Convert2Signed(TaskGroup *group, const Slice &slice, const Box2i &dw, int y) :
	Task(group),
	_slice(slice),
	_dw(dw),
	_y(y)
{

}


void
Convert2Signed::execute()
{
	switch(_slice.type)
	{
		case UINT10:
			ConvertRow<short, unsigned short, 512>();
		break;
		
		case UINT12:
			ConvertRow<short, unsigned short, 2048>();
		break;
		
		case UINT16:
			ConvertRow<short, unsigned short, 32768>();
		break;
		
		default:
			assert(false);
		break;
	}
}


template <typename STYPE, typename UTYPE, int DIFF>
void
Convert2Signed::ConvertRow()
{
	char *origin = _slice.base + (_y * _slice.yStride) + (_dw.min.x * _slice.xStride);
	
	UTYPE *in = (UTYPE *)origin;
	STYPE *out = (STYPE *)origin;
	
	const int in_step = _slice.xStride / sizeof(UTYPE);
	const int out_step = _slice.xStride / sizeof(STYPE);
	
	assert(in_step == out_step);
	
	for(int x = _dw.min.x; x <= _dw.max.x; x++)
	{
		*out = (int)*in - DIFF;
		
		in += in_step;
		out += out_step;
	}
}


static void
ConvertToSigned(const FrameBuffer &frameBuffer)
{
	const Box2i &dw = frameBuffer.dataWindow();

	TaskGroup taskGroup;

	for(FrameBuffer::ConstIterator i = frameBuffer.begin(); i != frameBuffer.end(); ++i)
	{
		const Slice &slice = i.slice();

		for(int y = dw.min.y; y <= dw.max.y; y++)
		{
			ThreadPool::addGlobalTask(new Convert2Signed(&taskGroup, slice, dw, y));
		}
	}
}


void
DiracCodec::compress(const FrameBuffer &frame)
{
	int bit_depth = 8;

	SchroVideoFormat *format = schro_encoder_get_video_format(_encoder);
	
	if(format)
	{
		if(format->luma_excursion == 1023)
			bit_depth = 10;
		else if(format->luma_excursion == 4095)
			bit_depth = 12;
		else
			assert(format->luma_excursion == 255);
		
		free(format);
	}
	else
		assert(false);
	
	const SchroFrameFormat schro_format = (bit_depth > 8 ? SCHRO_FRAME_FORMAT_S16_444 : SCHRO_FRAME_FORMAT_U8_444);
	
	SchroFrame *schro_frame = schro_frame_new_and_alloc(NULL, schro_format, frame.width(), frame.height());;
	
	if(schro_frame)
	{
		const MoxFiles::PixelType pixelType = (bit_depth == 12 ? MoxFiles::UINT12 :
												bit_depth == 10 ? MoxFiles::UINT10 :
												MoxFiles::UINT8);
		
		FrameBuffer schro_buffer(frame.width(), frame.height());
		
		if(MoxMxf::RGBADescriptor *rgb_descriptor = dynamic_cast<MoxMxf::RGBADescriptor *>(_descriptor))
		{
			// RGB
			const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = rgb_descriptor->getPixelLayout();
			
			assert(pixelLayout.size() == 3);
			
			for(int i = 0; i < pixelLayout.size(); i++)
			{
				if(pixelLayout[i].code != 'F')
				{
					const std::string chan_name(1, pixelLayout[i].code);
					
					const ptrdiff_t xStride = (schro_format == SCHRO_FRAME_FORMAT_S16_444 ? sizeof(short) : sizeof(unsigned char));
					
					schro_buffer.insert(chan_name, Slice(pixelType, (char *)schro_frame->components[i].data, xStride, schro_frame->components[i].stride));
				}
			}
		}
		else if(MoxMxf::CDCIDescriptor *cdci_descriptor = dynamic_cast<MoxMxf::CDCIDescriptor *>(_descriptor))
		{
			// YCbCr
			schro_buffer.insert("Y", Slice(pixelType, (char *)schro_frame->components[0].data, sizeof(unsigned char), schro_frame->components[0].stride));
			schro_buffer.insert("Cb", Slice(pixelType, (char *)schro_frame->components[1].data, sizeof(unsigned char), schro_frame->components[0].stride));
			schro_buffer.insert("Cr", Slice(pixelType, (char *)schro_frame->components[2].data, sizeof(unsigned char), schro_frame->components[1].stride));
		}
		else
			throw MoxMxf::ArgExc("Couldn't resolve descriptor");
			
		schro_buffer.copyFromFrame(frame);
		
		//if(schro_format == SCHRO_FRAME_FORMAT_S16_444)
		//{
		//	ConvertToSigned(schro_buffer);
		//}
		
		schro_encoder_push_frame(_encoder, schro_frame);
		
		encoder_pull();
	}
	else
		assert(false);
}


void
DiracCodec::decompress(const DataChunk &data)
{
	schro_decoder_reset(_decoder);

	unsigned char *buf = data.Data;
	ptrdiff_t buf_pos = 0;
	
	while(buf_pos <= (data.Size - SCHRO_PARSE_HEADER_SIZE) && buf[0] == 'B' && buf[1] == 'B' && buf[2] == 'C' && buf[3] == 'D')
	{
		unsigned int parse_code = buf[4];
		
		const int buf_len = (buf[5] << 24) + (buf[6] << 16) + (buf[7] << 8) + buf[8];
		
		if(buf_len < SCHRO_PARSE_HEADER_SIZE)
			break;
		
		SchroBuffer *schro_buf = schro_buffer_new_with_data(buf, buf_len);
		
		schro_decoder_push(_decoder, schro_buf);
		
		buf += buf_len;
		buf_pos += buf_len;
	}
	
	schro_decoder_push_end_of_stream(_decoder);
	
	decoder_pull();
}


void
DiracCodec::end_of_stream()
{
	if(_encoder != NULL)
	{
		schro_encoder_end_of_stream(_encoder);
		
		encoder_pull();
	}
	else if(_decoder != NULL)
	{
		schro_decoder_push_end_of_stream(_decoder);
		
		decoder_pull();
	}
	else
		assert(false); // huh?
}


void
DiracCodec::encoder_pull()
{
	if(_encoder != NULL)
	{
		DataChunkPtr data;
	
		bool go = true;
		
		while(go)
		{
			SchroStateEnum state = schro_encoder_wait(_encoder);
			
			if(state == SCHRO_STATE_HAVE_BUFFER || state == SCHRO_STATE_END_OF_STREAM)
			{
				int n_decodable_frames = -1;
			
				SchroBuffer *buffer = schro_encoder_pull(_encoder, &n_decodable_frames);
				
				if(buffer)
				{
					assert(buffer->length >= SCHRO_PARSE_HEADER_SIZE);
					
					const int parse_code = buffer->data[4];
					
					if(SCHRO_PARSE_CODE_IS_SEQ_HEADER(parse_code) ||
						SCHRO_PARSE_CODE_IS_AUXILIARY_DATA(parse_code) ||
						SCHRO_PARSE_CODE_IS_PICTURE(parse_code))
					{
						if(!data)
						{
							data = new DataChunk(buffer->length);
							
							data->Set(buffer->length, buffer->data);
						}
						else
						{
							data->Append(buffer->length, buffer->data);
						}
						
						if(SCHRO_PARSE_CODE_IS_PICTURE(parse_code))
							go = false; // end this data packet after we get a picture
					}
					else
						assert(!SCHRO_PARSE_CODE_IS_END_OF_SEQUENCE(parse_code) &&
								!SCHRO_PARSE_CODE_IS_PADDING(parse_code));
					
					schro_buffer_unref(buffer);
				}
				else
					assert(false);
					
				if(state == SCHRO_STATE_END_OF_STREAM)
					go = false;
			}
			else
			{
				assert(state == SCHRO_STATE_NEED_FRAME);
				assert(state != SCHRO_STATE_AGAIN); // yeah, redundant
			
				go = false;
			}
		}
		
		
		if(data)
			storeData(data);
	}
	else
		assert(false); // only call on encoder
}


class Convert2Unsigned : public Task
{
  public:
	Convert2Unsigned(TaskGroup *group, const Slice &slice, const Box2i &dw, int y);
	virtual ~Convert2Unsigned() {}
	
	virtual void execute();

  private:
	template <typename UTYPE, typename STYPE, int DIFF>
	void ConvertRow();

  private:
	const Slice &_slice;
	const Box2i &_dw;
	const int _y;
};


Convert2Unsigned::Convert2Unsigned(TaskGroup *group, const Slice &slice, const Box2i &dw, int y) :
	Task(group),
	_slice(slice),
	_dw(dw),
	_y(y)
{

}


void
Convert2Unsigned::execute()
{
	switch(_slice.type)
	{
		case UINT10:
			ConvertRow<unsigned short, short, 512>();
		break;
		
		case UINT12:
			ConvertRow<unsigned short, short, 2048>();
		break;
		
		case UINT16:
			ConvertRow<unsigned short, short, 32768>();
		break;
		
		default:
			assert(false);
		break;
	}
}


template <typename UTYPE, typename STYPE, int DIFF>
void
Convert2Unsigned::ConvertRow()
{
	char *origin = _slice.base + (_y * _slice.yStride) + (_dw.min.x * _slice.xStride);
	
	STYPE *in = (STYPE *)origin;
	UTYPE *out = (UTYPE *)origin;
	
	const int in_step = _slice.xStride / sizeof(STYPE);
	const int out_step = _slice.xStride / sizeof(UTYPE);
	
	assert(in_step == out_step);
	
	for(int x = _dw.min.x; x <= _dw.max.x; x++)
	{
		*out = (int)*in + DIFF;
		
		in += in_step;
		out += out_step;
	}
}


static void
ConvertToUnsigned(const FrameBuffer &frameBuffer)
{
	const Box2i &dw = frameBuffer.dataWindow();

	TaskGroup taskGroup;

	for(FrameBuffer::ConstIterator i = frameBuffer.begin(); i != frameBuffer.end(); ++i)
	{
		const Slice &slice = i.slice();

		for(int y = dw.min.y; y <= dw.max.y; y++)
		{
			ThreadPool::addGlobalTask(new Convert2Unsigned(&taskGroup, slice, dw, y));
		}
	}
}


void
DiracCodec::decoder_pull()
{
	bool go = true;
	
	while(go)
	{
		int state = schro_decoder_wait(_decoder);
	
		const unsigned int width = _descriptor->getStoredWidth();
		const unsigned int height = _descriptor->getStoredHeight();
			
		if(state == SCHRO_DECODER_FIRST_ACCESS_UNIT)
		{
			SchroVideoFormat *format = schro_decoder_get_video_format(_decoder);
			
			if(format)
			{
				assert(format->width == width);
				assert(format->height == height);
				
				assert(format->chroma_format == SCHRO_CHROMA_444);
				
				free(format);
			}
			else
				assert(false);
		}
		else if(state == SCHRO_DECODER_NEED_BITS)
		{
			go = false;
		}
		else if(state == SCHRO_DECODER_NEED_FRAME)
		{
			SchroFrameFormat frame_format = SCHRO_FRAME_FORMAT_U8_444;
			
			if(MoxMxf::RGBADescriptor *rgb_descriptor = dynamic_cast<MoxMxf::RGBADescriptor *>(_descriptor))
			{
				const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = rgb_descriptor->getPixelLayout();
				
				assert(pixelLayout.size() == 3);
				
				for(int i = 0; i < pixelLayout.size(); i++)
				{
					if(pixelLayout[i].code != 'F')
					{
						if(pixelLayout[i].depth > 8)
						{
							frame_format = SCHRO_FRAME_FORMAT_S16_444;
							break;
						}
					}
				}
			}
			else if(MoxMxf::CDCIDescriptor *cdci_descriptor = dynamic_cast<MoxMxf::CDCIDescriptor *>(_descriptor))
			{
				assert(false); // getting bit depth yet
			}
			else
				assert(false);
				
			SchroFrame *schro_frame = schro_frame_new_and_alloc(NULL, frame_format, width, height);
			
			schro_decoder_add_output_picture(_decoder, schro_frame);
		}
		else if(state == SCHRO_DECODER_OK)
		{
			//SchroTag *tag = schro_decoder_get_picture_tag(_decoder);
			
			SchroFrame *schro_frame = schro_decoder_pull(_decoder);
			
			if(schro_frame != NULL)
			{
				const SchroFrameFormat frame_format = schro_frame->components[0].format;
				
				const int frame_depth = (SCHRO_FRAME_FORMAT_DEPTH(frame_format) == SCHRO_FRAME_FORMAT_DEPTH_S16 ? 16 :
											SCHRO_FRAME_FORMAT_DEPTH(frame_format) == SCHRO_FRAME_FORMAT_DEPTH_S32 ? 32 :
											8);
			
				FrameBuffer schro_frameBuffer(width, height);
							
				FrameBufferPtr frame_buffer = new FrameBuffer(width, height);
			
				if(MoxMxf::RGBADescriptor *rgb_descriptor = dynamic_cast<MoxMxf::RGBADescriptor *>(_descriptor))
				{
					const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = rgb_descriptor->getPixelLayout();
					
					assert(pixelLayout.size() == 3);
					
					for(int i = 0; i < pixelLayout.size(); i++)
					{
						if(pixelLayout[i].code != 'F')
						{
							const std::string chan_name(1, pixelLayout[i].code);
							
							const PixelType pixelType = PixelTypeFromBits(pixelLayout[i].depth);
							
							const ptrdiff_t schro_xStride = (frame_depth > 8 ? sizeof(short) : sizeof(unsigned char));
							
							assert(frame_format == schro_frame->components[i].format); // all channels use same format
							
							schro_frameBuffer.insert(chan_name, Slice(pixelType, (char *)schro_frame->components[i].data, schro_xStride, schro_frame->components[i].stride));
							
							
							const size_t xStride = (pixelLayout[i].depth > 8 ? sizeof(unsigned short) : sizeof(unsigned char));
							
							const size_t rowbytes = width * xStride;
							const size_t mem_size = height * rowbytes;
							
							DataChunkPtr chan_data = new DataChunk(mem_size);
							
							frame_buffer->insert(chan_name, Slice(pixelType, (char *)chan_data->Data, xStride, rowbytes));
							
							frame_buffer->attachData(chan_data);
						}
					}
				}
				else if(MoxMxf::CDCIDescriptor *cdci_descriptor = dynamic_cast<MoxMxf::CDCIDescriptor *>(_descriptor))
				{
					assert(false); // not handling different bit depths yet
				
					const char *chans[3] = {"Y", "Cb", "Cr"};
					
					for(int i = 0; i < 3; i++)
					{
						const ptrdiff_t xStride = (frame_depth == 16 ? sizeof(short) : sizeof(unsigned char));
						
						schro_frameBuffer.insert(chans[i], Slice(MoxFiles::UINT8, (char *)schro_frame->components[i].data, xStride, schro_frame->components[i].stride));
						
						const size_t rowbytes = width * xStride;
						const size_t mem_size = height * rowbytes;
						
						DataChunkPtr chan_data = new DataChunk(mem_size);
						
						frame_buffer->insert(chans[i], Slice(MoxFiles::UINT8, (char *)chan_data->Data, xStride, rowbytes));
						
						frame_buffer->attachData(chan_data);
					}
				}
				
				//if(frame_depth > 8)
				//	ConvertToUnsigned(schro_frameBuffer);
				
				frame_buffer->copyFromFrame(schro_frameBuffer);
				
				storeFrame(frame_buffer);
				
				schro_frame_unref(schro_frame);
			
				go = false;
			}
			else
				assert(false);
		}
		else if(state == SCHRO_DECODER_EOS)
		{
			assert(false);
		}
		else if(state == SCHRO_DECODER_ERROR)
		{
			assert(false);
			
			go = false;
		}
	}
}


bool
DiracCodecInfo::canCompressType(PixelType pixelType) const
{
	return (pixelType == UINT8);
}


ChannelCapabilities
DiracCodecInfo::getChannelCapabilites() const
{
	return (Channels_RGB);
}


VideoCodec * 
DiracCodecInfo::createCodec(const Header &header, const ChannelList &channels) const
{
	return new DiracCodec(header, channels);
}

VideoCodec * 
DiracCodecInfo::createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const
{
	return new DiracCodec(descriptor, header, channels);
}


} //namespace


