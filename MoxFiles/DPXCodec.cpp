/*
 *  DPXCodec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 5/12/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#include <MoxFiles/DPXCodec.h>

#include <MoxFiles/MemoryFile.h>

#include "DPX.h"

namespace MoxFiles
{

DPXCodec::DPXCodec(const Header &header, const ChannelList &channels) :
	VideoCodec(header, channels),
	_descriptor(header.frameRate(), header.width(), header.height(), MoxMxf::VideoDescriptor::VideoCodecDPX),
	_pixelAspectRatio(header.pixelAspectRatio()),
	_frameRate(header.frameRate())
{
	setWindows(_descriptor, header);
	
	const Channel *r_channel = channels.findChannel("R");
	const Channel *a_channel = channels.findChannel("A");
	
	if(r_channel == NULL)
		throw MoxMxf::ArgExc("Expected RGB(A) channels");
	
	
	_channels = (a_channel != NULL ? DPX_RGBA : DPX_RGB);
	
	
	const unsigned int bit_depth = PixelBits(r_channel->type);
	
	_depth = (bit_depth == 8 ? DPX_8 :
				bit_depth == 10 ? DPX_10 :
				bit_depth == 12 ? DPX_12 :
				bit_depth == 16 ? DPX_16 :
				DPX_10);
	
	
	MoxMxf::RGBADescriptor::RGBALayout layout;
	
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('R', bit_depth));
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('G', bit_depth));
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('B', bit_depth));
	
	if(_channels == DPX_RGBA)
		layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('A', bit_depth));
	
	_descriptor.setPixelLayout(layout);
}


DPXCodec::DPXCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<const MoxMxf::RGBADescriptor &>(descriptor)),
	_pixelAspectRatio(header.pixelAspectRatio()),
	_frameRate(header.frameRate()),
	_depth(DPX_10),
	_channels(DPX_RGB)
{
	const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = _descriptor.getPixelLayout();
	
	for(MoxMxf::RGBADescriptor::RGBALayout::const_iterator i = pixelLayout.begin(); i != pixelLayout.end(); ++i)
	{
		const MoxMxf::RGBADescriptor::RGBALayoutItem &item = *i;
		
		if(item.code == 'A')
			_channels = DPX_RGBA;
		
		if(item.depth == 8)
			_depth = DPX_8;
		else if(item.depth == 10)
			_depth = DPX_10;
		else if(item.depth == 12)
			_depth = DPX_12;
		else if(item.depth == 16)
			_depth = DPX_16;
		else
			throw MoxMxf::InputExc("Unexpected bit depth");
	}
	
	
	const PixelType pixel_type = (_depth == DPX_8 ? UINT8 :
									_depth == DPX_10 ? UINT10 :
									_depth == DPX_12 ? UINT12 :
									_depth == DPX_16 ? UINT16 :
									UINT10);
									
	channels.insert("R", Channel(pixel_type));
	channels.insert("G", Channel(pixel_type));
	channels.insert("B", Channel(pixel_type));
	
	if(_channels == DPX_RGBA)
		channels.insert("A", Channel(pixel_type));
}


DPXCodec::~DPXCodec()
{

}


class DPXCodec_OutStream : public OutStream
{
  public:
	DPXCodec_OutStream(MemoryFile &file);
	virtual ~DPXCodec_OutStream() {}
	
	//virtual bool Open(const char *fn);
	//virtual void Close();
	virtual size_t Write(void * buf, const size_t size);
	virtual bool Seek(long offset, Origin origin);
	virtual void Flush() {}
	
  private:
	MemoryFile &_file;
};


DPXCodec_OutStream::DPXCodec_OutStream(MemoryFile &file) :
	_file(file)
{
	assert(_file.FileTell() == 0);
	assert(_file.FileSize() == 0);
}


size_t
DPXCodec_OutStream::Write(void * buf, const size_t size)
{
	return _file.FileWrite((unsigned char *)buf, size);
}


bool
DPXCodec_OutStream::Seek(long offset, Origin origin)
{
	const MoxMxf::UInt64 absolute_offset = (origin == kCurrent ? (_file.FileTell() + offset) :
											origin == kEnd ? (_file.FileSize() - 1 + offset) :
											offset); // kStart
											
	if(absolute_offset <= _file.FileSize())
	{
		_file.FileSeek(absolute_offset);
		
		return true;
	}
	else
		return false;
}


void
DPXCodec::compress(const FrameBuffer &frame)
{
	const PixelType pixel_type = (_depth == DPX_8 ? MoxFiles::UINT8 : MoxFiles::UINT16);
	const size_t bytes_per_subpixel = PixelSize(pixel_type);
	const int num_channels = (_channels == DPX_RGBA ? 4 : 3);
	
	const Box2i dataW = dataWindow();
	
	const int width = (dataW.max.x - dataW.min.x + 1);
	const int height = (dataW.max.y - dataW.min.y + 1);
	
	const size_t bytes_per_pixel = bytes_per_subpixel * num_channels;
	const size_t rowbytes = bytes_per_pixel * width;
	const size_t buffer_size = rowbytes * height;
	
	
	DataChunk data(buffer_size);
	
	FrameBuffer frame_buffer(dataW);
		
	char *origin = (char *)data.Data;
	
	frame_buffer.insert("R", Slice(pixel_type, origin + (bytes_per_subpixel * 0), bytes_per_pixel, rowbytes));
	frame_buffer.insert("G", Slice(pixel_type, origin + (bytes_per_subpixel * 1), bytes_per_pixel, rowbytes));
	frame_buffer.insert("B", Slice(pixel_type, origin + (bytes_per_subpixel * 2), bytes_per_pixel, rowbytes));
	
	if(_channels == DPX_RGBA)
		frame_buffer.insert("A", Slice(pixel_type, origin + (bytes_per_subpixel * 3), bytes_per_pixel, rowbytes));
	
	
	frame_buffer.copyFromFrame(frame);
	
	
	MemoryFile file(buffer_size);
	
	DPXCodec_OutStream ostream(file);
	
	dpx::Writer dpx;
	
	dpx.SetOutStream(&ostream);
	
	dpx.SetFileInfo("MOX",	// fileName
					NULL,			// creation time (set by libdpx)
					"DPXcodec, part of MOX", // creator
					NULL,			// project
					NULL,			// copyright
					~0,				// encryption key (0xffffffff means no encryption)
					false);			// don't swap byte order
	
	dpx.SetImageInfo(width, height);
	
	dpx.header.SetAspectRatio(0, _pixelAspectRatio.Numerator);
	dpx.header.SetAspectRatio(1, _pixelAspectRatio.Denominator);
	
	dpx.header.SetFrameRate((dpx::R32)_frameRate.Numerator / (dpx::R32)_frameRate.Denominator);
	
	//dpx.header.SetInterlace(info->field_label->type == FIEL_Type_FRAME_RENDERED ? 0 : 1);
	
	const unsigned int bit_depth = (_depth == DPX_8 ? 8 :
									_depth == DPX_10 ? 10 :
									_depth == DPX_12 ? 12 :
									_depth == DPX_16 ? 16 :
									10);
	
	const dpx::Descriptor dpx_desc = (_channels == DPX_RGBA ? dpx::kRGBA : dpx::kRGB);
	const dpx::Packing packing = ((bit_depth == 8 || bit_depth == 16) ? dpx::kPacked : dpx::kFilledMethodA);
	
	const dpx::Characteristic transfer = dpx::kLogarithmic;
	const dpx::Characteristic colorimetric = dpx::kLogarithmic;
	
	dpx.SetElement(0, dpx_desc, bit_depth, transfer, colorimetric, packing);
	
	
	const bool wrote_header = dpx.WriteHeader();
	
	if(!wrote_header)
		throw MoxMxf::ArgExc("Error writing header");
	

	const dpx::DataSize size = (pixel_type == MoxFiles::UINT16 ? dpx::kWord : dpx::kByte);
	
	const bool wrote_element = dpx.WriteElement(0, data.Data, size);
	
	if(!wrote_element)
		throw MoxMxf::ArgExc("Error writing image");
	
	
	const bool wrote_finish = dpx.Finish();
	
	if(!wrote_finish)
		throw MoxMxf::ArgExc("Error writing finish");
	
	
	storeData( file.getDataChunk() );
}


class DPXCodec_InStream : public InStream
{
  public:
	DPXCodec_InStream(MemoryFile &file);
	virtual ~DPXCodec_InStream() {}
	
	//virtual bool Open(const char * fn);
	//virtual void Close();
	virtual void Rewind() { Seek(0, kStart); }
	
	virtual size_t Read(void * buf, const size_t size);
	virtual size_t ReadDirect(void * buf, const size_t size);
	
	virtual bool EndOfFile() const;
	virtual bool Seek(long offset, Origin origin);

  private:
	MemoryFile &_file;
};


DPXCodec_InStream::DPXCodec_InStream(MemoryFile &file) :
	_file(file)
{
	_file.FileSeek(0);
}


size_t
DPXCodec_InStream::Read(void * buf, const size_t size)
{
	return _file.FileRead((unsigned char *)buf, size);
}


size_t
DPXCodec_InStream::ReadDirect(void * buf, const size_t size)
{
	return Read(buf, size);
}


bool
DPXCodec_InStream::EndOfFile() const
{
	return (_file.FileTell() >= _file.FileSize());
}


bool
DPXCodec_InStream::Seek(long offset, Origin origin)
{
	const MoxMxf::UInt64 absolute_offset = (origin == kCurrent ? (_file.FileTell() + offset) :
											origin == kEnd ? (_file.FileSize() - 1 + offset) :
											offset); // kStart
											
	if(absolute_offset < _file.FileSize())
	{
		_file.FileSeek(absolute_offset);
		
		return true;
	}
	else
		return false;
}

void
DPXCodec::decompress(const DataChunk &data)
{
	MemoryFile file(data);
	
	DPXCodec_InStream istream(file);
	
	dpx::Reader dpx;
	
	dpx.SetInStream(&istream);
	
	const bool header_read = dpx.ReadHeader();
	
	if(header_read)
	{
		const int dpx_width = dpx.header.Width();
		const int dpx_height = dpx.header.Height();
		const int dpx_channels = dpx.header.ImageElementComponentCount(0);
		
		assert(dpx_width == _descriptor.getStoredWidth());
		assert(dpx_height == _descriptor.getStoredHeight());
		assert(dpx_channels == (_channels == DPX_RGBA ? 4 : 3));
		
		
		const Box2i dataW = dataWindow();
		
		if(dpx_width != (dataW.max.x - dataW.min.x + 1))
			throw MoxMxf::InputExc("Stored data is wrong width");

		if(dpx_height != (dataW.max.y - dataW.min.y + 1))
			throw MoxMxf::InputExc("Stored data is wrong height");
		
		
		const size_t pixsize = (_depth == DPX_8 ? sizeof(unsigned char) : sizeof(unsigned short));
		const size_t rowbytes = dpx_width * pixsize * dpx_channels;
		const size_t buffer_size = rowbytes * dpx_height;
		
		DataChunkPtr frame_data = new DataChunk(buffer_size);
		
		char *origin = (char *)frame_data->Data;
		
		FrameBufferPtr frame_buffer = new FrameBuffer(dataW);
		
		const PixelType pixel_type = (_depth == DPX_8 ? MoxFiles::UINT8 : MoxFiles::UINT16);
		const size_t bytes_per_subpixel = PixelSize(pixel_type);
		const size_t bytes_per_pixel = bytes_per_subpixel * dpx_channels;
	
		frame_buffer->insert("R", Slice(pixel_type, origin + (bytes_per_subpixel * 0), bytes_per_pixel, rowbytes));
		frame_buffer->insert("G", Slice(pixel_type, origin + (bytes_per_subpixel * 1), bytes_per_pixel, rowbytes));
		frame_buffer->insert("B", Slice(pixel_type, origin + (bytes_per_subpixel * 2), bytes_per_pixel, rowbytes));
		
		if(_channels == DPX_RGBA)
			frame_buffer->insert("A", Slice(pixel_type, origin + (bytes_per_subpixel * 3), bytes_per_pixel, rowbytes));
			
		frame_buffer->attachData(frame_data);
		
		
		dpx::Block block(0, 0, dpx_width - 1, dpx_height - 1);
		
		const dpx::Descriptor dpx_desc = dpx.header.ImageDescriptor(0);
		const dpx::DataSize dpx_size = (_depth == DPX_8 ? dpx::kByte : dpx::kWord);
		
		const bool image_read = dpx.ReadBlock((void *)origin, dpx_size, block, dpx_desc);
		
		if(image_read)
		{
			storeFrame(frame_buffer);
		}
		else
			assert(false);
	}
	else
		assert(false);
}


bool
DPXCodecInfo::canCompressType(PixelType pixelType) const
{
	return (pixelType == UINT8 || pixelType == UINT10 || pixelType == UINT12 || pixelType == UINT16);
}


ChannelCapabilities
DPXCodecInfo::getChannelCapabilites() const
{
	return (Channels_RGB | Channels_RGBA);
}


VideoCodec * 
DPXCodecInfo::createCodec(const Header &header, const ChannelList &channels) const
{
	return new DPXCodec(header, channels);
}

VideoCodec * 
DPXCodecInfo::createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const
{
	return new DPXCodec(descriptor, header, channels);
}


} // namespace
