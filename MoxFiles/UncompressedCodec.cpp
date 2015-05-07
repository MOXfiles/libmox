/*
 *  UncompressedCodec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/22/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/UncompressedCodec.h>

namespace MoxFiles
{

static unsigned int
PixelLayoutDepth(PixelType type)
{
	// see SMPTE 377M E.2.46
	switch(type)
	{
		case UINT8:
			return 8;
		
		case UINT10:
			return 10;
			
		case UINT12:
			return 12;
			
		case UINT16:
			return 16;
			
		case UINT16A:
			assert(false); // lets not store Adobe 16-bit
			return 16;
		
		case UINT32:
			return 32;
		
		case HALF:
			assert(false); // uncompressed MXF doesn't know about half
			return 16;
		
		case FLOAT:
			return 254;
	}
	
	throw MoxMxf::ArgExc("Unknown pixel type");
}

static unsigned int
PixelLayoutBits(PixelType type)
{
	const unsigned int depth = PixelLayoutDepth(type);
	
	return (depth == 254 ? 32 : depth);
}


UncompressedCodec::UncompressedCodec(const Header &header, const ChannelList &channels) :
	VideoCodec(header, channels),
	_descriptor(header.frameRate(), header.width(), header.height()),
	_padding(0)
{
	if(const Channel *r_channel = channels.findChannel("R"))
	{
		_channels.push_back( ChannelBits("R", 'R', r_channel->type) );
	}
	
	if(const Channel *g_channel = channels.findChannel("G"))
	{
		_channels.push_back( ChannelBits("G", 'G', g_channel->type) );
	}

	if(const Channel *b_channel = channels.findChannel("B"))
	{
		_channels.push_back( ChannelBits("B", 'B', b_channel->type) );
	}

	if(const Channel *a_channel = channels.findChannel("A"))
	{
		_channels.push_back( ChannelBits("A", 'A', a_channel->type) );
	}

	if(_channels.empty())
		throw MoxMxf::ArgExc("Need some RGBA channels please");
	
	
	MoxMxf::RGBADescriptor::PixelLayout layout;
	
	unsigned int bits_per_pixel = 0;
	
	for(std::vector<ChannelBits>::const_iterator i = _channels.begin(); i != _channels.end(); ++i)
	{
		layout.push_back(i->code);
		layout.push_back( PixelLayoutDepth(i->type) );
		
		bits_per_pixel += PixelLayoutBits(i->type);
	}
	
	if(bits_per_pixel % 8 != 0)
	{
		_padding = 8 - bits_per_pixel % 8;
	
		layout.push_back('F'); // fill
		layout.push_back(_padding);
	}
	
	// to mark the end
	layout.push_back(0);
	layout.push_back(0);
	
	_descriptor.setPixelLayout(layout);
}


UncompressedCodec::UncompressedCodec(MoxMxf::VideoDescriptor *descriptor, Header &header, const ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<MoxMxf::RGBADescriptor &>(*descriptor))
{
	assert(header.width() == _descriptor.getWidth());
	assert(header.height() == _descriptor.getHeight());
	
	const MoxMxf::RGBADescriptor::PixelLayout &pixelLayout = _descriptor.getPixelLayout();
	
	const int num_channels = pixelLayout.size() / 2;
	
	for(int i = 0; i < num_channels; i++)
	{
		const int n = i * 2;
	
		assert(pixelLayout[n] == 'R' || pixelLayout[n] == 'G' || pixelLayout[n] == 'B' || pixelLayout[n] == 'A');
		assert(pixelLayout[n+1] == 8);
		
		_channels.push_back(ChannelBits(std::string(1, pixelLayout[n]), pixelLayout[n], MoxFiles::UINT8));
	}
}

UncompressedCodec::~UncompressedCodec()
{

}


void
UncompressedCodec::compress(FrameBufferPtr frame)
{
	if(frame->width() != _descriptor.getWidth() || frame->height() != _descriptor.getHeight())
		throw MoxMxf::ArgExc("Frame buffer doesn't match expected dimensions");

	unsigned int bits_per_pixel = 0;
	
	for(std::vector<ChannelBits>::const_iterator i = _channels.begin(); i != _channels.end(); ++i)
	{
		bits_per_pixel += PixelLayoutBits(i->type);
		
		assert(PixelLayoutBits(i->type) % 8 == 0); // not dealing with weird bits yet
	}
	
	bits_per_pixel += _padding;
	
	assert(bits_per_pixel % 8 == 0);
		

	const size_t rowbytes = frame->width() * (bits_per_pixel >> 3);
	const size_t data_size = rowbytes * frame->height();
	
	DataChunkPtr data = new DataChunk(data_size);
	
	char *origin = (char *)data->Data;
	
	FrameBuffer stored_frame(frame->width(), frame->height());
	
	for(std::vector<ChannelBits>::const_iterator i = _channels.begin(); i != _channels.end(); ++i)
	{
		stored_frame.insert(i->name, Slice(i->type, origin, (bits_per_pixel >> 3), rowbytes));
		
		origin += (PixelLayoutBits(i->type) >> 3);
	}
	
	stored_frame.copyFromFrame(*frame);
	
	
	storeData(data);
}


void
UncompressedCodec::decompress(const DataChunk &data)
{
	const size_t rowbytes = _descriptor.getWidth() * _channels.size() * sizeof(unsigned char);
	const size_t data_size = rowbytes * _descriptor.getHeight();
	
	assert(data_size == data.Size);
	
	FrameBuffer encoded_frameBuffer(_descriptor.getWidth(), _descriptor.getHeight());
	
	
	FrameBufferPtr exported_frameBuffer = new FrameBuffer(_descriptor.getWidth(), _descriptor.getHeight());
	
	DataChunkPtr data_to_store = new DataChunk(data_size);
	
	exported_frameBuffer->attachData(data_to_store);
	
	
	char *encoded_origin = (char *)data.Data;
	
	char *exported_origin = (char *)data_to_store->Data;
	
	for(int i = 0; i < _channels.size(); i++)
	{
		assert(_channels[i].type == MoxFiles::UINT8);
		
		const size_t pixel_size = sizeof(unsigned char);
		
		encoded_frameBuffer.insert(_channels[i].name, Slice(MoxFiles::UINT8, encoded_origin + (i * pixel_size), _channels.size() * pixel_size, rowbytes));
		
		exported_frameBuffer->insert(_channels[i].name, Slice(MoxFiles::UINT8, exported_origin + (i * pixel_size), _channels.size() * pixel_size, rowbytes));
	}
	
	exported_frameBuffer->copyFromFrame(encoded_frameBuffer);
	
	storeFrame(exported_frameBuffer);
}


ChannelCapabilities
UncompressedCodecInfo::getChannelCapabilites() const
{
	return (Channels_RGB | Channels_RGBA | Channels_A);
}


VideoCodec * 
UncompressedCodecInfo::createCodec(const Header &header, const ChannelList &channels) const
{
	return new UncompressedCodec(header, channels);
}

VideoCodec * 
UncompressedCodecInfo::createCodec(MoxMxf::VideoDescriptor *descriptor, Header &header, const ChannelList &channels) const
{
	return new UncompressedCodec(descriptor, header, channels);
}



//bool
//UncompressedCodecInfo::canCompressType(PixelType pixelType) const
//{
//	// SMPTE 377M E.2.46
//	// Can store anything but 16-bit float
//	return (pixelType != HALF);
//}

} // namespace
