/*
 *  PNGCodec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 6/24/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/PNGCodec.h>

#include <MoxFiles/MemoryFile.h>

#include "png.h"



namespace MoxFiles
{

PNGCodec::PNGCodec(const Header &header, const ChannelList &channels) :
	VideoCodec(header, channels),
	_descriptor(header.frameRate(), header.width(), header.height(), MoxMxf::VideoDescriptor::VideoCodecPNG)
{
	setWindows(_descriptor, header);


	const Channel *r_channel = channels.findChannel("R");
	const Channel *a_channel = channels.findChannel("A");
	const Channel *y_channel = channels.findChannel("Y");
	
	if(r_channel != NULL)
	{
		_channels = (a_channel != NULL ? PNG_RGBA : PNG_RGB);
		
		_depth = (PixelBits(r_channel->type) > 8 ? PNG_16 : PNG_8);
	}
	else if(y_channel != NULL)
	{
		_channels = (a_channel != NULL ? PNG_YA : PNG_Y);
		
		_depth = (PixelBits(y_channel->type) > 8 ? PNG_16 : PNG_8);
	}
	else
		throw MoxMxf::ArgExc("Invalid PNG channels configuration");

	
	const unsigned int bits_per_pixel = (_depth == PNG_16 ? 16 : 8);
	
	
	MoxMxf::RGBADescriptor::RGBALayout layout;
	
	if(_channels == PNG_RGB || _channels == PNG_RGBA)
	{
		layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('R', bits_per_pixel));
		layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('G', bits_per_pixel));
		layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('B', bits_per_pixel));
		
		if(_channels == PNG_RGBA)
			layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('A', bits_per_pixel));
	}
	else if(_channels == PNG_Y || _channels == PNG_YA)
	{
		layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('Y', bits_per_pixel));
		
		if(_channels == PNG_YA)
			layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('A', bits_per_pixel));
	}
	else
		throw MoxMxf::LogicExc("Ummmm...");
		
	_descriptor.setPixelLayout(layout);
}


PNGCodec::PNGCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<const MoxMxf::RGBADescriptor &>(descriptor)),
	_depth(PNG_8),
	_channels(PNG_RGB)
{
	bool haveR = false, haveA = false, haveY = false;
	
	
	const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = _descriptor.getPixelLayout();
	
	for(MoxMxf::RGBADescriptor::RGBALayout::const_iterator i = pixelLayout.begin(); i != pixelLayout.end(); ++i)
	{
		const MoxMxf::RGBADescriptor::RGBALayoutItem &item = *i;
		
		if(item.code == 'R')
		{
			haveR = true;
		}
		else if(item.code == 'A')
		{
			haveA = true;
		}
		else if(item.code == 'Y')
		{
			haveY = true;
		}
		
		if(item.depth > 8)
			_depth = PNG_16;
	}
	
	const PixelType pixel_type = (_depth == PNG_16 ? UINT16 : UINT8);
	
	if(haveY)
	{
		_channels = (haveA ? PNG_YA : PNG_Y);
		
		channels.insert("Y", Channel(pixel_type));
	}
	else
	{
		_channels = (haveA ? PNG_RGBA : PNG_RGB);
		
		channels.insert("R", Channel(pixel_type));
		channels.insert("G", Channel(pixel_type));
		channels.insert("B", Channel(pixel_type));
	}
	
	if(haveA)
		channels.insert("A", Channel(pixel_type));
}


PNGCodec::~PNGCodec()
{

}


static void png_replace_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	MemoryFile *file = (MemoryFile *)png_get_io_ptr(png_ptr);
	
	file->FileWrite(data, length);
}

static void png_replace_flush(png_structp png_ptr)
{
	MemoryFile *file = (MemoryFile *)png_get_io_ptr(png_ptr);
	
	// nothing
}

void
PNGCodec::compress(const FrameBuffer &frame)
{
	const PixelType pixel_type = (_depth == PNG_16 ? MoxFiles::UINT16 : MoxFiles::UINT8);
	const size_t bytes_per_subpixel = PixelSize(pixel_type);
	const int bit_depth = (_depth == PNG_16 ? 16 : 8);
	
	const int num_channels = (_channels == PNG_RGB ? 3 :
								_channels == PNG_RGBA ? 4 :
								_channels == PNG_Y ? 1 :
								_channels == PNG_YA ? 2 :
								4);
	
	const Box2i dataW = dataWindow();
	
	const int width = (dataW.max.x - dataW.min.x + 1);
	const int height = (dataW.max.y - dataW.min.y + 1);
	
	const size_t bytes_per_pixel = bytes_per_subpixel * num_channels;
	const size_t rowbytes = bytes_per_pixel * width;
	const size_t buffer_size = rowbytes * height;
	
	
	DataChunk data(buffer_size);
	
	FrameBuffer frame_buffer(dataW);
		
	
	char *origin = (char *)data.Data;
	
	if(_channels == PNG_RGB || _channels == PNG_RGBA)
	{
		frame_buffer.insert("R", Slice(pixel_type, origin + (bytes_per_subpixel * 0), bytes_per_pixel, rowbytes));
		frame_buffer.insert("G", Slice(pixel_type, origin + (bytes_per_subpixel * 1), bytes_per_pixel, rowbytes));
		frame_buffer.insert("B", Slice(pixel_type, origin + (bytes_per_subpixel * 2), bytes_per_pixel, rowbytes));
		
		if(_channels == PNG_RGBA)
			frame_buffer.insert("A", Slice(pixel_type, origin + (bytes_per_subpixel * 3), bytes_per_pixel, rowbytes));
	}
	else
	{
		frame_buffer.insert("Y", Slice(pixel_type, origin + (bytes_per_subpixel * 0), bytes_per_pixel, rowbytes));
		
		if(_channels == PNG_YA)
			frame_buffer.insert("A", Slice(pixel_type, origin + (bytes_per_subpixel * 1), bytes_per_pixel, rowbytes));
	}
	
	frame_buffer.copyFromFrame(frame);
	
	
	MemoryFile file(buffer_size);
	
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		
	png_infop info_ptr = png_create_info_struct(png_ptr);
	
	png_set_write_fn(png_ptr, (void *)&file, png_replace_write_data, png_replace_flush);
	
	// this is how libpng handles errors
#ifdef PNG_SETJMP_SUPPORTED
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		
		throw MoxMxf::IoExc("Problem writing PNG file");
	}
#endif

	const int color_type = (_channels == PNG_RGBA ? PNG_COLOR_TYPE_RGBA :
							_channels == PNG_RGB ? PNG_COLOR_TYPE_RGB :
							_channels == PNG_YA ? PNG_COLOR_TYPE_GA :
							_channels == PNG_Y ? PNG_COLOR_TYPE_GRAY :
							PNG_COLOR_TYPE_RGB);

	png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type,
					PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	//png_set_filter(png_ptr, PNG_FILTER_TYPE_DEFAULT, gOptions.filter);
	//png_set_compression_level(png_ptr, gOptions.compression);
	//png_set_compression_strategy(png_ptr, gOptions.strategy);
	
	png_write_info(png_ptr, info_ptr);
	
	
	png_bytepp row_pointers = (png_bytepp)png_malloc(png_ptr, height * sizeof(png_bytep));
					
	for(int row = 0; row < height; row++)
	{
		row_pointers[row] = (png_bytep)( origin + (row * rowbytes) );
	}
	
	png_write_image(png_ptr, row_pointers);
	
	
	png_free(png_ptr, (void *)row_pointers);
	
	png_write_end(png_ptr, info_ptr);
	
	png_destroy_write_struct(&png_ptr, &info_ptr);
	
	
	storeData( file.getDataChunk() );
}


static void png_replace_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	MemoryFile *file = (MemoryFile *)png_get_io_ptr(png_ptr);
	
	file->FileRead(data, length);
}


void
PNGCodec::decompress(const DataChunk &data)
{
	MemoryFile file(data);
	
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	
	png_set_read_fn(png_ptr, &file, png_replace_read_data);
	
	// this is how libpng handles errors
#ifdef PNG_SETJMP_SUPPORTED
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		
		throw MoxMxf::IoExc("Problem reading PNG file");
	}
#endif

	png_read_info(png_ptr, info_ptr);
	
	int color_type, bit_depth, interlace_type;
	png_uint_32 width, height;

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		&interlace_type, NULL, NULL);


	const PixelType pixel_type = (bit_depth > 8 ? MoxFiles::UINT16 : MoxFiles::UINT8);
	const size_t bytes_per_subpixel = PixelSize(pixel_type);
	
	const int num_channels = (color_type & PNG_COLOR_MASK_COLOR) ?
								((color_type & PNG_COLOR_MASK_ALPHA) ? 4 : 3) :
								((color_type & PNG_COLOR_MASK_ALPHA) ? 2 : 1);
	
	const size_t bytes_per_pixel = bytes_per_subpixel * num_channels;
	const size_t rowbytes = bytes_per_pixel * width;
	const size_t buffer_size = rowbytes * height;
	
	
	const Box2i dataW = dataWindow();
	
	if(width != (dataW.max.x - dataW.min.x + 1))
		throw MoxMxf::InputExc("Stored data is wrong width");

	if(height != (dataW.max.y - dataW.min.y + 1))
		throw MoxMxf::InputExc("Stored data is wrong height");

	DataChunkPtr frame_data = new DataChunk(buffer_size);
	
	FrameBufferPtr frame_buffer = new FrameBuffer(dataW);
	
	frame_buffer->attachData(frame_data);
	
	
	char *origin = (char *)frame_data->Data;
	
	if( (color_type & PNG_COLOR_MASK_COLOR) )
	{
		frame_buffer->insert("R", Slice(pixel_type, origin + (bytes_per_subpixel * 0), bytes_per_pixel, rowbytes));
		frame_buffer->insert("G", Slice(pixel_type, origin + (bytes_per_subpixel * 1), bytes_per_pixel, rowbytes));
		frame_buffer->insert("B", Slice(pixel_type, origin + (bytes_per_subpixel * 2), bytes_per_pixel, rowbytes));
		
		if( (color_type & PNG_COLOR_MASK_ALPHA) )
			frame_buffer->insert("A", Slice(pixel_type, origin + (bytes_per_subpixel * 3), bytes_per_pixel, rowbytes));
	}
	else
	{
		frame_buffer->insert("Y", Slice(pixel_type, origin + (bytes_per_subpixel * 0), bytes_per_pixel, rowbytes));
		
		if( (color_type & PNG_COLOR_MASK_ALPHA) )
			frame_buffer->insert("A", Slice(pixel_type, origin + (bytes_per_subpixel * 1), bytes_per_pixel, rowbytes));
	}
	
	
	png_bytepp row_pointers = (png_bytepp)png_malloc(png_ptr, height * sizeof(png_bytep));
	
	for(int row = 0; row < height; row++)
	{
		row_pointers[row] = (png_bytep)( origin + (row * rowbytes) );
	}
	
	
	png_read_image(png_ptr, row_pointers);
	
	
	png_free(png_ptr, (void *)row_pointers);
	
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	
	
	storeFrame(frame_buffer);
}


bool
PNGCodecInfo::canCompressType(PixelType pixelType) const
{
	return (pixelType == UINT8 || pixelType == UINT16);
}


ChannelCapabilities
PNGCodecInfo::getChannelCapabilites() const
{
	return (Channels_RGB | Channels_RGBA | Channels_Y | Channels_YA | Channels_A);
}


VideoCodec * 
PNGCodecInfo::createCodec(const Header &header, const ChannelList &channels) const
{
	return new PNGCodec(header, channels);
}

VideoCodec * 
PNGCodecInfo::createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const
{
	return new PNGCodec(descriptor, header, channels);
}


} // namespace

