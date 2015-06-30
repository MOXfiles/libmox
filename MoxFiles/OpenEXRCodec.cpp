/*
 *  OpenEXRCodec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 6/29/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/OpenEXRCodec.h>

#include <MoxFiles/MemoryFile.h>

#include "ImfHybridInputFile.h"
#include "ImfOutputFile.h"

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
		case 32:
			return MoxFiles::UINT32;
		
		case 253:
			return MoxFiles::HALF;
			
		case 254:
			return MoxFiles::FLOAT;
	}
	
	throw MoxMxf::ArgExc("Bad EXR pixel type");
}


OpenEXRCodec::OpenEXRCodec(const Header &header, const ChannelList &channels) :
	VideoCodec(header, channels),
	_descriptor(header.frameRate(), header.width(), header.height(), MoxMxf::VideoDescriptor::VideoCodecOpenEXR),
	_exr_header(NULL)
{
	setWindows(_descriptor, header);
	
	
	_exr_header = new Imf::Header(displayWindow(), dataWindow());
	
	MoxMxf::RGBADescriptor::RGBALayout layout;
	
	for(ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i)
	{
		const char *name = i.name();
		const Channel &chan = i.channel();
		
		assert(strlen(name) == 1); // only doing RGBA for now
		assert(name[0] == 'R' || name[0] == 'G' || name[0] == 'B' || name[0] == 'A');
		
		layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem(name[0], PixelLayoutDepth(chan.type)));
		
		
		const Imf::PixelType exr_pixel_type = (chan.type == MoxFiles::UINT32 ? Imf::UINT :
												chan.type == MoxFiles::FLOAT ? Imf::FLOAT :
												Imf::HALF);
		
		_exr_header->channels().insert(name, Imf::Channel(exr_pixel_type, chan.xSampling, chan.ySampling));
	}
		
	_descriptor.setPixelLayout(layout);
}


OpenEXRCodec::OpenEXRCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<const MoxMxf::RGBADescriptor &>(descriptor)),
	_exr_header(NULL)
{
	const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = _descriptor.getPixelLayout();
	
	for(MoxMxf::RGBADescriptor::RGBALayout::const_iterator i = pixelLayout.begin(); i != pixelLayout.end(); ++i)
	{
		const MoxMxf::RGBADescriptor::RGBALayoutItem &item = *i;
		
		std::string name(item.code, 1);
		
		channels.insert(name.c_str(), Channel(PixelTypeFromBits(item.depth)));
	}
}


OpenEXRCodec::~OpenEXRCodec()
{
	delete _exr_header;
}


class MoxOStream : public Imf::OStream
{
  public:
	MoxOStream(MemoryFile &file);
	virtual ~MoxOStream() {}
	
	virtual void write(const char c[/*n*/], int n);
	virtual Imf::Int64 tellp();
	virtual void seekp(Imf::Int64 pos);
	
  private:
	MemoryFile &_file;
};


MoxOStream::MoxOStream(MemoryFile &file) :
	Imf::OStream("MoxOStream"),
	_file(file)
{
	assert(_file.FileTell() == 0);
	assert(_file.FileSize() == 0);
}


void
MoxOStream::write(const char c[/*n*/], int n)
{
	MoxMxf::UInt64 written = _file.FileWrite((unsigned char *)c, n);
	
	if(written != n)
		throw MoxMxf::LogicExc("Memory write failed?!?");
}


Imf::Int64
MoxOStream::tellp()
{
	return _file.FileTell();
}


void
MoxOStream::seekp(Imf::Int64 pos)
{
	_file.FileSeek(pos);
}


void
OpenEXRCodec::compress(const FrameBuffer &frame)
{
	const Box2i &dataW = _exr_header->dataWindow();
	
	assert(dataW == dataWindow());
	
	const int width = dataW.max.x - dataW.min.x + 1;
	const int height = dataW.max.y - dataW.min.y + 1;

	FrameBuffer converted_frameBufer(dataW);
	
	Imf::FrameBuffer exr_frameBuffer;
	
	for(Imf::ChannelList::ConstIterator i = _exr_header->channels().begin(); i != _exr_header->channels().end(); ++i)
	{
		const char *name = i.name();
		const Imf::Channel &chan = i.channel();
	
		const MoxFiles::PixelType pixel_type = (chan.type == Imf::UINT ? MoxFiles::UINT32 :
												chan.type == Imf::FLOAT ? MoxFiles::FLOAT :
												MoxFiles::HALF);
												
		const size_t subpixel_size = PixelSize(pixel_type);
		const size_t rowbytes = subpixel_size * width;
		const size_t data_size = rowbytes * height;
		
		DataChunkPtr chan_buffer = new DataChunk(data_size);
		
		converted_frameBufer.attachData(chan_buffer);
		
		converted_frameBufer.insert(name, MoxFiles::Slice(pixel_type, (char *)chan_buffer->Data, subpixel_size, rowbytes,
									chan.xSampling, chan.ySampling));
		
		exr_frameBuffer.insert(name, Imf::Slice(chan.type, (char *)chan_buffer->Data, subpixel_size, rowbytes,
								chan.xSampling, chan.ySampling));
	}
	
	converted_frameBufer.copyFromFrame(frame);
	
	
	
	MemoryFile mem_file;
		
	{
		MoxOStream stream(mem_file);
		
		Imf::OutputFile file(stream, *_exr_header);
		
		file.setFrameBuffer(exr_frameBuffer);
		
		file.writePixels(height);
		
		// Imf::OutputFile must go out of scope before we write
	}
	
	storeData( mem_file.getDataChunk() );
}


class MoxIStream : public Imf::IStream
{
  public:
	MoxIStream(MemoryFile &file);
	virtual ~MoxIStream() {}
	
	virtual bool isMemoryMapped() const { return true; }
	virtual bool read(char c[/*n*/], int n);
	virtual char *readMemoryMapped(int n);
	virtual Imf::Int64 tellg();
	virtual void seekg(Imf::Int64 pos);

  private:
	MemoryFile &_file;
};


MoxIStream::MoxIStream(MemoryFile &file) :
	Imf::IStream("MoxIStream"),
	_file(file)
{
	_file.FileSeek(0);
}


bool
MoxIStream::read(char c[/*n*/], int n)
{
	MoxMxf::UInt64 bytes = _file.FileRead((unsigned char *)c, n);
	
	return (bytes == n);
}


char *
MoxIStream::readMemoryMapped(int n)
{
	return (char *)_file.MemoryRead(n);
}


Imf::Int64
MoxIStream::tellg()
{
	return _file.FileTell();
}


void
MoxIStream::seekg(Imf::Int64 pos)
{
	_file.FileSeek(pos);
}


void
OpenEXRCodec::decompress(const DataChunk &data)
{
	MemoryFile mem_file(data);
	
	MoxIStream stream(mem_file);
	
	Imf::HybridInputFile file(stream);
	
	
	const Imath::Box2i &dataW = file.dataWindow();
	
	assert(dataW == dataWindow());
	
	const int width = dataW.max.x - dataW.min.x + 1;
	const int height = dataW.max.y - dataW.min.y + 1;
	
	
	FrameBufferPtr frameBuffer = new FrameBuffer(dataW);
	
	Imf::FrameBuffer exr_frameBuffer;
	
	for(Imf::ChannelList::ConstIterator i = file.channels().begin(); i != file.channels().end(); ++i)
	{
		const char *name = i.name();
		const Imf::Channel &chan = i.channel();
		
		MoxFiles::PixelType pixel_type = (chan.type == Imf::UINT ? MoxFiles::UINT32 :
											chan.type == Imf::FLOAT ? MoxFiles::FLOAT :
											MoxFiles::HALF);
											
		const size_t subpixel_size = PixelSize(pixel_type);
		const size_t rowbytes = subpixel_size * width;
		const size_t data_size = rowbytes * height;
		
		DataChunkPtr chan_buffer = new DataChunk(data_size);
		
		frameBuffer->attachData(chan_buffer);
		
		frameBuffer->insert(name, MoxFiles::Slice(pixel_type, (char *)chan_buffer->Data, subpixel_size, rowbytes,
								chan.xSampling, chan.ySampling));
								
		exr_frameBuffer.insert(name, Imf::Slice(chan.type, (char *)chan_buffer->Data, subpixel_size, rowbytes,
								chan.xSampling, chan.ySampling));
	}
	
	
	file.setFrameBuffer(exr_frameBuffer);
	
	file.readPixels(dataW.min.y, dataW.max.y);
	
	
	storeFrame(frameBuffer);
}


bool
OpenEXRCodecInfo::canCompressType(PixelType pixelType) const
{
	return (pixelType == MoxFiles::HALF || pixelType == MoxFiles::FLOAT || pixelType == MoxFiles::UINT32);
}


ChannelCapabilities
OpenEXRCodecInfo::getChannelCapabilites() const
{
	return Channels_All;
}


VideoCodec * 
OpenEXRCodecInfo::createCodec(const Header &header, const ChannelList &channels) const
{
	return new OpenEXRCodec(header, channels);
}

VideoCodec * 
OpenEXRCodecInfo::createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const
{
	return new OpenEXRCodec(descriptor, header, channels);
}


} // namespace
