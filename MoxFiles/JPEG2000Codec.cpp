/*
 *  JPEG2000Codec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 11/15/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#include <MoxFiles/JPEG2000Codec.h>

#include <MoxFiles/MemoryFile.h>
#include <MoxFiles/Thread.h>

#include "openjpeg.h"

#include <assert.h>
//#include <algorithm>

namespace MoxFiles
{

JPEG2000Codec::JPEG2000Codec(const Header &header, const ChannelList &channels) :
	VideoCodec(header, channels),
	_descriptor(header.frameRate(), header.width(), header.height(), MoxMxf::VideoDescriptor::VideoCodecJPEG2000)
{
	setWindows(_descriptor, header);
	
	const Channel *r_channel = channels.findChannel("R");
	const Channel *a_channel = channels.findChannel("A");
	
	if(r_channel == NULL)
		throw MoxMxf::ArgExc("Expected RGB(A) channels");
	
	
	_channels = (a_channel != NULL ? JP2_RGBA : JP2_RGB);
	
	
	const unsigned int bit_depth = PixelBits(r_channel->type);
	
	_depth = (bit_depth == 8 ? JP2_8 :
				bit_depth == 10 ? JP2_10 :
				bit_depth == 12 ? JP2_12 :
				bit_depth == 16 ? JP2_16 :
				JP2_8);
	
	
	MoxMxf::RGBADescriptor::RGBALayout layout;
	
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('R', bit_depth));
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('G', bit_depth));
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('B', bit_depth));
	
	if(_channels == JP2_RGBA)
		layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('A', bit_depth));
	
	_descriptor.setPixelLayout(layout);
	
	
	const int num_channels = layout.size();
	
	MoxMxf::VideoDescriptor::PictureComponentSizing components;
	
	for(int i=0; i < num_channels; i++)
	{
		MoxMxf::VideoDescriptor::PictureComponent comp;
		
		components.push_back(comp);
	}
	
	_descriptor.setPictureComponentSizing(components);
	
	
	_lossless = isLossless(header);
	_quality = (_lossless ? 100 : getQuality(header));
}


JPEG2000Codec::JPEG2000Codec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<const MoxMxf::RGBADescriptor &>(descriptor)),
	_depth(JP2_10),
	_channels(JP2_RGB)
{
	assert(_descriptor.getVideoCodec() == MoxMxf::VideoDescriptor::VideoCodecJPEG2000);

	const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = _descriptor.getPixelLayout();
	
	// TODO: JPEG 2000 could handle channels with different bit depths
	for(MoxMxf::RGBADescriptor::RGBALayout::const_iterator i = pixelLayout.begin(); i != pixelLayout.end(); ++i)
	{
		const MoxMxf::RGBADescriptor::RGBALayoutItem &item = *i;
		
		if(item.code == 'A')
			_channels = JP2_RGBA;
		
		if(item.depth == 8)
			_depth = JP2_8;
		else if(item.depth == 10)
			_depth = JP2_10;
		else if(item.depth == 12)
			_depth = JP2_12;
		else if(item.depth == 16)
			_depth = JP2_16;
		else
			throw MoxMxf::InputExc("Unexpected bit depth");
	}
	
	
	const PixelType pixel_type = (_depth == JP2_8 ? UINT8 :
									_depth == JP2_10 ? UINT10 :
									_depth == JP2_12 ? UINT12 :
									_depth == JP2_16 ? UINT16 :
									UINT10);
									
	channels.insert("R", Channel(pixel_type));
	channels.insert("G", Channel(pixel_type));
	channels.insert("B", Channel(pixel_type));
	
	if(_channels == JP2_RGBA)
		channels.insert("A", Channel(pixel_type));
}


JPEG2000Codec::~JPEG2000Codec()
{

}


static void
ErrorHandler(const char *msg, void *client_data)
{
	printf("openjpeg error: %s", msg);
}

static void
WarningHandler(const char *msg, void *client_data)
{
	printf("openjpeg warning: %s", msg);
}

static void
InfoHandler(const char *msg, void *client_data)
{
	printf("openjpeg info: %s", msg);
}


static OPJ_SIZE_T
OutputStreamRead(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	MemoryFile *file = (MemoryFile *)p_user_data;
	
	return file->FileRead((unsigned char *)p_buffer, p_nb_bytes);
}

static OPJ_SIZE_T
OutputStreamWrite(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	MemoryFile *file = (MemoryFile *)p_user_data;
	
	return file->FileWrite((const unsigned char *)p_buffer, p_nb_bytes);
}

static OPJ_OFF_T
OutputStreamSkip(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	MemoryFile *file = (MemoryFile *)p_user_data;
	
	const MoxMxf::UInt64 currentPos = file->FileTell();
	
	const MoxMxf::UInt64 newPos = (currentPos + p_nb_bytes);
	
	try
	{
		file->FileSeek(newPos);
		
		return p_nb_bytes;
	}
	catch(...)
	{
	
	}
	
	return -1;
}

static OPJ_BOOL
OutputStreamSeek(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	MemoryFile *file = (MemoryFile *)p_user_data;
	
	try
	{
		file->FileSeek(p_nb_bytes);
		
		return OPJ_TRUE;
	}
	catch(...)
	{
	
	}
	
	return OPJ_FALSE;
}


class CopyToJP2Buffer : public Task
{
  public:
	CopyToJP2Buffer(TaskGroup *group, const opj_image_comp_t &component, const Slice &slice, int y);
	~CopyToJP2Buffer() {}
	
	virtual void execute();
	
  private:
	const opj_image_comp_t &_comp;
	const Slice &_slice;
	const int _y;
	
	template <typename PIXTYPE>
	void CopyRow(OPJ_INT32 *out, const PIXTYPE *in, int inStep, int len);
};

CopyToJP2Buffer::CopyToJP2Buffer(TaskGroup *group, const opj_image_comp_t &component, const Slice &slice, int y) :
	Task(group),
	_comp(component),
	_slice(slice),
	_y(y)
{

}

void
CopyToJP2Buffer::execute()
{
	OPJ_INT32 *outRow = (_comp.data + (_y * _comp.w));
	const int outDepth = _comp.prec;
	
	const char *inRow = (_slice.base + (_y * _slice.yStride));
	const int inStep = (_slice.xStride / PixelSize(_slice.type));
	const int inDepth = PixelBits(_slice.type);
	
	assert(outDepth == inDepth);
	assert(!_comp.sgnd);
	
	if(_slice.type == UINT8)
	{
		CopyRow<unsigned char>(outRow, (unsigned char *)inRow, inStep, _comp.w);
	}
	else if(_slice.type == UINT10 || _slice.type == UINT12 || _slice.type == UINT16)
	{
		CopyRow<unsigned short>(outRow, (unsigned short *)inRow, inStep, _comp.w);
	}
	else
		assert(false);
}

template <typename PIXTYPE>
void
CopyToJP2Buffer::CopyRow(OPJ_INT32 *out, const PIXTYPE *in, int inStep, int len)
{
	for(int x=0; x < len; x++)
	{
		*out = *in;
		
		out++;
		in += inStep;
	}
}


void
JPEG2000Codec::compress(const FrameBuffer &frame)
{
	//const PixelType pixel_type = (_depth == JP2_8 ? MoxFiles::UINT8 : MoxFiles::UINT16);
	//const size_t bytes_per_subpixel = PixelSize(pixel_type);
	const int num_channels = (_channels == JP2_RGBA ? 4 : 3);
	
	const Box2i dataW = dataWindow();
	
	const int width = (dataW.max.x - dataW.min.x + 1);
	const int height = (dataW.max.y - dataW.min.y + 1);
	
	
	MemoryFile file;
	
	OPJ_BOOL success = OPJ_TRUE;
	
	opj_stream_t *stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_FALSE);
	
	if(stream)
	{
		opj_stream_set_user_data(stream, &file, NULL);
		opj_stream_set_read_function(stream, OutputStreamRead);
		opj_stream_set_write_function(stream, OutputStreamWrite);
		opj_stream_set_skip_function(stream, OutputStreamSkip);
		opj_stream_set_seek_function(stream, OutputStreamSeek);
		
		
		// TODO: investigate skip
		// Only writing J2K format right now because JP2 is trying to skip past the end of
		// the file.  Is that really a good idea?  What does fseek() do?
		
		const OPJ_CODEC_FORMAT format = OPJ_CODEC_J2K;
		
		opj_codec_t *codec = opj_create_compress(format);
		
		if(codec)
		{
			opj_set_error_handler(codec, ErrorHandler, NULL);
			opj_set_warning_handler(codec, WarningHandler, NULL);
			opj_set_info_handler(codec, InfoHandler, NULL);
			
			//opj_codec_set_threads(codec, NumberOfCPUs());
			
			const OPJ_UINT32 depth = (_depth == JP2_8 ? 8 :
										_depth == JP2_10 ? 10 :
										_depth == JP2_12 ? 12 :
										_depth == JP2_16 ? 16 :
										8);
										
			const OPJ_UINT32 bpp = (_depth == JP2_8 ? 8 : 16);
			
			
			opj_image_cmptparm_t compParam[4];
			
			for(int i=0; i < num_channels; i++)
			{
				opj_image_cmptparm_t &param = compParam[i];
				
				param.dx = 1;
				param.dy = 1;
				param.w = width;
				param.h = height;
				param.x0 = 0;
				param.y0 = 0;
				param.prec = depth;
				param.bpp = bpp;
				param.sgnd = OPJ_FALSE;
			}
			
			
			const OPJ_COLOR_SPACE colorSpace = OPJ_CLRSPC_SRGB;
			
			opj_image_t *image = opj_image_create(num_channels, compParam, colorSpace);
			
			if(image)
			{
				image->x0 = 0;
				image->y0 = 0;
				image->x1 = width;
				image->y1 = height;
				
				
				const PixelType nativeType = (_depth == JP2_8 ? UINT8 :
												_depth == JP2_10 ? UINT10 :
												_depth == JP2_12 ? UINT12 :
												_depth == JP2_16 ? UINT16 :
												UINT8);
												
				FrameBuffer tempBuffer(dataW);
				
				const size_t tempPixelSize = PixelSize(nativeType);
				const size_t tempRowBytes = (width * tempPixelSize);
				const size_t tempChannelSize = (height * tempRowBytes);
												
				const char *chanNames[4] = { "R", "G", "B", "A" };
				
				for(int i=0; i < num_channels; i++)
				{
					DataChunkPtr chanData = new DataChunk(tempChannelSize);
					
					tempBuffer.insert(chanNames[i], Slice(nativeType, (char *)chanData->Data, tempPixelSize, tempRowBytes));
					
					tempBuffer.attachData(chanData);
				}
				
				tempBuffer.copyFromFrame(frame);
				
				
				{
					TaskGroup taskGroup;
				
					for(OPJ_UINT32 i=0U; i < num_channels; i++)
					{
						const Slice *slice = tempBuffer.findSlice(chanNames[i]);
					
						if(slice != NULL)
						{
							const opj_image_comp_t &comp = image->comps[i];
							
							assert(dataW.min.x == 0 && dataW.min.y == 0);
							
							for(int y=0; y < height; y++)
							{
								ThreadPool::addGlobalTask(new CopyToJP2Buffer(&taskGroup, comp, *slice, y));
							}
						}
						else
							assert(false);
					}
				}
				
				
				opj_cparameters_t params;
				
				opj_set_default_encoder_parameters(&params);
				
				
				if(_lossless)
				{
					params.cp_disto_alloc = OPJ_TRUE;
					params.tcp_numlayers = 1;
				}
				else
				{
					/*params.cp_disto_alloc = OPJ_TRUE;
					params.tcp_numlayers = 10;
					
					const float min_rate = (400 - (4 * (_quality - 1)));
					
					for(int i=0; i < params.tcp_numlayers; i++)
					{
						params.tcp_rates[i] = (min_rate * (params.tcp_numlayers - i));
					}*/
					
					
					params.cp_fixed_quality = OPJ_TRUE;
					params.tcp_numlayers = 10;
					
					const float psnr_boost = ((float)_quality / 4.0);
					
					for(int i=0; i < params.tcp_numlayers; i++)
					{
						params.tcp_distoratio[i] = 25.0 + (psnr_boost / (params.tcp_numlayers - i));
					}
					
					
					params.irreversible = OPJ_TRUE;
				}
				
				
				// too scared for
				// success = opj_setup_encoder(codec, &params, image) &&
				//				opj_start_compress(codec, image, stream) &&
				//				opj_encode(codec, stream) &&
				//				opj_end_compress(codec, stream);
				
				success = opj_setup_encoder(codec, &params, image);
				
				if(success)
				{
					success = opj_start_compress(codec, image, stream);
					
					if(success)
					{
						success = opj_encode(codec, stream);
						
						if(success)
						{
							success = opj_end_compress(codec, stream);
						}
					}
				}
				
				opj_image_destroy(image);
			}
			else
				success = OPJ_FALSE;
				
			opj_destroy_codec(codec);
		}
		else
			success = OPJ_FALSE;
		
		opj_stream_destroy(stream);
	}
	else
		success = OPJ_FALSE;
	
	
	if(!success)
		throw MoxMxf::ArgExc("JPEG 2000 compression error");
	
	
	storeData( file.getDataChunk() );
}


static OPJ_SIZE_T
InputStreamRead(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	MemoryFile *file = (MemoryFile *)p_user_data;
	
	return file->FileRead((unsigned char *)p_buffer, p_nb_bytes);
}

static OPJ_OFF_T
InputStreamSkip(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	MemoryFile *file = (MemoryFile *)p_user_data;
	
	const MoxMxf::UInt64 currentPos = file->FileTell();
	
	const MoxMxf::UInt64 newPos = (currentPos + p_nb_bytes);
	
	try
	{
		file->FileSeek(newPos);
		
		return p_nb_bytes;
	}
	catch(...)
	{
	
	}
	
	return -1;
}

static OPJ_BOOL
InputStreamSeek(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	MemoryFile *file = (MemoryFile *)p_user_data;
	
	try
	{
		file->FileSeek(p_nb_bytes);
		
		return OPJ_TRUE;
	}
	catch(...)
	{
	
	}
	
	return OPJ_FALSE;
}


class CopyFromJP2Buffer : public Task
{
  public:
	CopyFromJP2Buffer(TaskGroup *group, const Slice &slice, const opj_image_comp_t &component, int y);
	~CopyFromJP2Buffer() {}
	
	virtual void execute();
	
  private:
	const Slice &_slice;
	const opj_image_comp_t &_comp;
	const int _y;
	
	template <typename PIXTYPE>
	void CopyRow(PIXTYPE *out, int outStep, const OPJ_INT32 *in, int len);
};

CopyFromJP2Buffer::CopyFromJP2Buffer(TaskGroup *group, const Slice &slice, const opj_image_comp_t &component, int y) :
	Task(group),
	_slice(slice),
	_comp(component),
	_y(y)
{

}

void
CopyFromJP2Buffer::execute()
{
	const char *outRow = (_slice.base + (_y * _slice.yStride));
	const int outStep = (_slice.xStride / PixelSize(_slice.type));
	const int outDepth = PixelBits(_slice.type);
	
	OPJ_INT32 *inRow = (_comp.data + (_y * _comp.w));
	const int inDepth = _comp.prec;
	
	assert(outDepth == inDepth);
	assert(!_comp.sgnd);
	
	if(_slice.type == UINT8)
	{
		CopyRow<unsigned char>((unsigned char *)outRow, outStep, inRow, _comp.w);
	}
	else if(_slice.type == UINT10 || _slice.type == UINT12 || _slice.type == UINT16)
	{
		CopyRow<unsigned short>((unsigned short *)outRow, outStep, inRow, _comp.w);
	}
	else
		assert(false);
}

template <typename PIXTYPE>
void
CopyFromJP2Buffer::CopyRow(PIXTYPE *out, int outStep, const OPJ_INT32 *in, int len)
{
	for(int x=0; x < len; x++)
	{
		*out = *in;
		
		out += outStep;
		in++;
	}
}


void
JPEG2000Codec::decompress(const DataChunk &data)
{
	bool success = true;
	
	opj_stream_t *stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_TRUE);
	
	if(stream)
	{
		MemoryFile file(data);
		
		opj_stream_set_user_data(stream, &file, NULL);
		opj_stream_set_user_data_length(stream, data.Size);
		opj_stream_set_read_function(stream, InputStreamRead);
		opj_stream_set_skip_function(stream, InputStreamSkip);
		opj_stream_set_seek_function(stream, InputStreamSeek);
		
		
		opj_codec_t *codec = opj_create_decompress(OPJ_CODEC_J2K); // what about OPJ_CODEC_JP2?
		
		if(codec)
		{
			opj_set_error_handler(codec, ErrorHandler, NULL);
			opj_set_warning_handler(codec, WarningHandler, NULL);
			opj_set_info_handler(codec, InfoHandler, NULL);
			
			opj_codec_set_threads(codec, ThreadPool::globalThreadPool().numThreads());
			
			
			opj_image_t *image = NULL;
			
			OPJ_BOOL imageRead = opj_read_header(stream, codec, &image);
			
			if(imageRead && image != NULL)
			{
				imageRead = opj_decode(codec, stream, image);
			
				if(imageRead)
				{
					assert(_descriptor.getStoredWidth() == image->x1);
					assert(_descriptor.getStoredHeight() == image->y1);
					assert((_channels == JP2_RGBA ? 4 : 3) == image->numcomps);
					
					const Box2i dataW = dataWindow();
					
					const int width = (dataW.max.x - dataW.min.x + 1);
					const int height = (dataW.max.y - dataW.min.y + 1);
					
					const PixelType nativeType = (_depth == JP2_8 ? UINT8 :
													_depth == JP2_10 ? UINT10 :
													_depth == JP2_12 ? UINT12 :
													_depth == JP2_16 ? UINT16 :
													UINT8);
													
					const size_t pixsize = PixelSize(nativeType);
					const size_t rowbytes = width * pixsize;
					const size_t buffer_size = rowbytes * height;
					
					const int num_channels = (_channels == JP2_RGBA ? 4 : 3);
					
					
					const char *chanNames[4] = { "R", "G", "B", "A" };
					
					FrameBufferPtr frame_buffer = new FrameBuffer(dataW);
					
					for(int i=0; i < num_channels; i++)
					{
						DataChunkPtr channel_data = new DataChunk(buffer_size);
						
						frame_buffer->insert(chanNames[i], Slice(nativeType, (char *)channel_data->Data, pixsize, rowbytes));
						
						frame_buffer->attachData(channel_data);
					}
					
					{
						TaskGroup taskGroup;
					
						for(OPJ_UINT32 i=0U; i < num_channels; i++)
						{
							const Slice *slice = frame_buffer->findSlice(chanNames[i]);
						
							if(slice != NULL)
							{
								const opj_image_comp_t &comp = image->comps[i];
								
								assert(width == comp.w && height == comp.h);
								assert(dataW.min.x == 0 && dataW.min.y == 0);
								
								for(int y=0; y < height; y++)
								{
									ThreadPool::addGlobalTask(new CopyFromJP2Buffer(&taskGroup, *slice, comp, y));
								}
							}
							else
								assert(false);
						}
					}
					
					storeFrame(frame_buffer);
				}
				else
					success = false;
			}
			else
				success = false;
				
			if(image)
				opj_image_destroy(image);
			
			opj_destroy_codec(codec);
		}
		else
			success = false;
			
		opj_stream_destroy(stream);
	}
	else
		success = false;
	
	
	if(!success)
		throw MoxMxf::ArgExc("JPEG 2000 decompression error");
}


bool
JPEG2000CodecInfo::canCompressType(PixelType pixelType) const
{
	return (pixelType == UINT8 || pixelType == UINT10 || pixelType == UINT12 || pixelType == UINT16);
}


ChannelCapabilities
JPEG2000CodecInfo::getChannelCapabilites() const
{
	return (Channels_RGB | Channels_RGBA);
}


VideoCodec * 
JPEG2000CodecInfo::createCodec(const Header &header, const ChannelList &channels) const
{
	return new JPEG2000Codec(header, channels);
}

VideoCodec * 
JPEG2000CodecInfo::createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const
{
	return new JPEG2000Codec(descriptor, header, channels);
}


} // namespace
