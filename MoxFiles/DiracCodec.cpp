/*
 *  DiracCodec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 7/28/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/DiracCodec.h>


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
			assert(false); // not yet
			return 10;
			
		case MoxFiles::UINT12:
			assert(false); // not yet
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
			assert(false); // uncompressed MXF doesn't know about half
			return 253;
		
		case MoxFiles::FLOAT:
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
	_descriptor(header.frameRate(), header.width(), header.height(), MoxMxf::VideoDescriptor::VideoCodecDirac),
	_encoder(NULL),
	_decoder(NULL)
{
	setWindows(_descriptor, header);
	
	
	schro_init();
	
	_encoder = schro_encoder_new();
	
	if(_encoder == NULL)
		throw MoxMxf::NullExc("Error creating encoder");
	
	
	// see struct SchroEncoderSettings in schroencoder.c
	schro_encoder_setting_set_double(_encoder, "gop_structure", SCHRO_ENCODER_GOP_INTRA_ONLY);
	schro_encoder_setting_set_double(_encoder, "rate_control", SCHRO_ENCODER_RATE_CONTROL_LOSSLESS);
	//schro_encoder_setting_set_double(_encoder, "rate_control", SCHRO_ENCODER_RATE_CONTROL_CONSTANT_QUALITY);
	//schro_encoder_setting_set_double(_encoder, "quality", 1.0); // 0.0 - 10.0

	SchroVideoFormat *format = schro_encoder_get_video_format(_encoder); // this allocates a new version with a copy of the internal struct
	
	if(format)
	{
		//schro_video_format_set_std_video_format(format, SCHRO_VIDEO_FORMAT_HD1080P_60);
		//schro_video_format_set_std_frame_rate(format, 1);
		
		format->width = header.width();
		format->height = header.height();
		format->chroma_format = SCHRO_CHROMA_444;
		
		schro_video_format_set_std_signal_range(format, SCHRO_SIGNAL_RANGE_8BIT_VIDEO);
		
		format->interlaced = false;
		//format->top_field_first = true;
		
		format->frame_rate_numerator = header.frameRate().Numerator;
		format->frame_rate_denominator = header.frameRate().Denominator;
		
		format->aspect_ratio_numerator = header.pixelAspectRatio().Numerator;
		format->aspect_ratio_denominator = header.pixelAspectRatio().Denominator;
		
		format->colour_primaries = SCHRO_COLOUR_PRIMARY_HDTV;
		format->colour_matrix = SCHRO_COLOUR_MATRIX_HDTV; // SCHRO_COLOUR_MATRIX_REVERSIBLE
		format->transfer_function = SCHRO_TRANSFER_CHAR_TV_GAMMA; // SCHRO_TRANSFER_CHAR_LINEAR
		
		schro_encoder_set_video_format(_encoder, format);
		
		free(format);
	}
	else
		assert(false);
	
	assert(_descriptor.getHorizontalSubsampling() == 1);
	assert(_descriptor.getVerticalSubsampling() == 1);
	assert(_descriptor.getComponentDepth() == 8);
	
	schro_debug_set_level(SCHRO_LEVEL_WARNING);
	
	schro_encoder_start(_encoder);
}


DiracCodec::DiracCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<const MoxMxf::CDCIDescriptor &>(descriptor)),
	_encoder(NULL),
	_decoder(NULL)
{
	schro_init();
	
	_decoder = schro_decoder_new();
	
	if(_decoder == NULL)
		throw MoxMxf::NullExc("Error creating decoder"); 
	
	//schro_decoder_set_skip_ratio(_decoder, 1.0);
	
	assert(_descriptor.getComponentDepth() == 8);
	assert(_descriptor.getAlphaSampleDepth() == 0);
	
	channels.insert("R", Channel(PixelTypeFromBits(8)));
	channels.insert("G", Channel(PixelTypeFromBits(8)));
	channels.insert("B", Channel(PixelTypeFromBits(8)));
}


DiracCodec::~DiracCodec()
{
	if(_encoder != NULL)
		schro_encoder_free(_encoder);
		
	if(_decoder != NULL)
		schro_decoder_free(_decoder);
}


void
DiracCodec::compress(const FrameBuffer &frame)
{
	SchroFrame *schro_frame = schro_frame_new_and_alloc(NULL, SCHRO_FRAME_FORMAT_U8_444, frame.width(), frame.height());
	
	if(schro_frame)
	{
		FrameBuffer schro_buffer(frame.width(), frame.height());
		
		schro_buffer.insert("Y", Slice(MoxFiles::UINT8, (char *)schro_frame->components[0].data, sizeof(unsigned char), schro_frame->components[0].stride));
		schro_buffer.insert("Cb", Slice(MoxFiles::UINT8, (char *)schro_frame->components[1].data, sizeof(unsigned char), schro_frame->components[0].stride));
		schro_buffer.insert("Cr", Slice(MoxFiles::UINT8, (char *)schro_frame->components[2].data, sizeof(unsigned char), schro_frame->components[1].stride));
		
		schro_buffer.copyFromFrame(frame);
		
		
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


void
DiracCodec::decoder_pull()
{
	bool go = true;
	
	while(go)
	{
		int state = schro_decoder_wait(_decoder);
	
		const unsigned int width = _descriptor.getStoredWidth();
		const unsigned int height = _descriptor.getStoredHeight();
			
		if(state == SCHRO_DECODER_FIRST_ACCESS_UNIT)
		{
			SchroVideoFormat *format = schro_decoder_get_video_format(_decoder);
			
			if(format)
			{
				assert(format->width == width);
				assert(format->height == height);
			
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
			SchroFrame *schro_frame = schro_frame_new_and_alloc(NULL, SCHRO_FRAME_FORMAT_U8_444, width, height);
			
			schro_decoder_add_output_picture(_decoder, schro_frame);
		}
		else if(state == SCHRO_DECODER_OK)
		{
			//SchroTag *tag = schro_decoder_get_picture_tag(_decoder);
			
			SchroFrame *schro_frame = schro_decoder_pull(_decoder);
			
			if(schro_frame != NULL)
			{
				FrameBuffer schro_frameBuffer(width, height);
							
				FrameBufferPtr frame_buffer = new FrameBuffer(width, height);
			
				const char *chans[3] = {"Y", "Cb", "Cr"};
				
				for(int i = 0; i < 3; i++)
				{
					schro_frameBuffer.insert(chans[i], Slice(MoxFiles::UINT8, (char *)schro_frame->components[i].data, sizeof(unsigned char), schro_frame->components[i].stride));
					
					const size_t rowbytes = width * sizeof(unsigned char);
					const size_t mem_size = height * rowbytes;
					
					DataChunkPtr chan_data = new DataChunk(mem_size);
					
					frame_buffer->insert(chans[i], Slice(MoxFiles::UINT8, (char *)chan_data->Data, sizeof(unsigned char), rowbytes));
					
					frame_buffer->attachData(chan_data);
				}
				
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
	return (pixelType == UINT8 || pixelType == UINT10 || pixelType == UINT12 || pixelType == UINT16);
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


