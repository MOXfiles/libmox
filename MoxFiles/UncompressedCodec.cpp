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
	ChannelList in_channels = channels;

	if(const Channel *r_channel = in_channels.findChannel("R"))
	{
		_channelVec.push_back( ChannelBits("R", 'R', r_channel->type) );
		
		in_channels.erase("R");
	}
	
	if(const Channel *g_channel = in_channels.findChannel("G"))
	{
		_channelVec.push_back( ChannelBits("G", 'G', g_channel->type) );
		
		in_channels.erase("G");
	}

	if(const Channel *b_channel = in_channels.findChannel("B"))
	{
		_channelVec.push_back( ChannelBits("B", 'B', b_channel->type) );
		
		in_channels.erase("B");
	}

	if(const Channel *a_channel = in_channels.findChannel("A"))
	{
		_channelVec.push_back( ChannelBits("A", 'A', a_channel->type) );
		
		in_channels.erase("A");
	}

	if(_channelVec.empty())
		throw MoxMxf::ArgExc("Need some RGBA channels please");
	
	assert(in_channels.size() == 0); // otherwise there was an extra channel in here
	
	
	MoxMxf::RGBADescriptor::RGBALayout layout;
	
	unsigned int bits_per_pixel = 0;
	
	for(std::vector<ChannelBits>::const_iterator i = _channelVec.begin(); i != _channelVec.end(); ++i)
	{
		layout.push_back( MoxMxf::RGBADescriptor::RGBALayoutItem(i->code, PixelLayoutDepth(i->type)) );
		
		bits_per_pixel += PixelLayoutBits(i->type);
	}
	
	if(bits_per_pixel % 8 != 0)
	{
		_padding = 8 - bits_per_pixel % 8;
	
		layout.push_back( MoxMxf::RGBADescriptor::RGBALayoutItem('F', _padding) ); // fill
	}
	
	_descriptor.setPixelLayout(layout);
}


UncompressedCodec::UncompressedCodec(MoxMxf::VideoDescriptor *descriptor, Header &header, ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<MoxMxf::RGBADescriptor &>(*descriptor)),
	_padding(0)
{
	assert(header.width() == _descriptor.getWidth());
	assert(header.height() == _descriptor.getHeight());
	
	const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = _descriptor.getPixelLayout();
	
	const int num_channels = pixelLayout.size();
	
	for(int i = 0; i < num_channels; i++)
	{
		assert(pixelLayout[i].code == 'R' || pixelLayout[i].code == 'G' || pixelLayout[i].code == 'B' || pixelLayout[i].code == 'A');
		assert(pixelLayout[i].depth == 8);
		
		if(pixelLayout[i].code == 'F')
		{
			assert(_padding == 0);
			
			_padding = pixelLayout[i].depth;
		}
		else
		{
			const std::string chan_name(1, pixelLayout[i].code);
			
			_channelVec.push_back(ChannelBits(chan_name, pixelLayout[i].code, MoxFiles::UINT8));
			
			channels.insert(chan_name, Channel(MoxFiles::UINT8));
		}
	}
}

UncompressedCodec::~UncompressedCodec()
{

}


void
UncompressedCodec::compress(const FrameBuffer &frame)
{
	if(frame.width() != _descriptor.getWidth() || frame.height() != _descriptor.getHeight())
		throw MoxMxf::ArgExc("Frame buffer doesn't match expected dimensions");

	unsigned int bits_per_pixel = 0;
	
	for(std::vector<ChannelBits>::const_iterator i = _channelVec.begin(); i != _channelVec.end(); ++i)
	{
		bits_per_pixel += PixelLayoutBits(i->type);
		
		assert(PixelLayoutBits(i->type) % 8 == 0); // not dealing with weird bits yet
	}
	
	bits_per_pixel += _padding;
	
	assert(bits_per_pixel % 8 == 0);
		

	const size_t pixel_size = (bits_per_pixel >> 3);
	const size_t rowbytes = frame.width() * pixel_size;
	const size_t data_size = rowbytes * frame.height();
	
	DataChunkPtr data = new DataChunk(data_size);
	
	char *origin = (char *)data->Data;
	
	FrameBuffer stored_frame(frame.width(), frame.height());
	
	for(std::vector<ChannelBits>::const_iterator i = _channelVec.begin(); i != _channelVec.end(); ++i)
	{
		stored_frame.insert(i->name, Slice(i->type, origin, pixel_size, rowbytes));
		
		origin += (PixelLayoutBits(i->type) >> 3);
	}
	
	stored_frame.copyFromFrame(frame);
	
	
	storeData(data);
}


void
UncompressedCodec::decompress(const DataChunk &data)
{
	unsigned int bits_per_pixel = 0;
	
	for(std::vector<ChannelBits>::const_iterator i = _channelVec.begin(); i != _channelVec.end(); ++i)
	{
		bits_per_pixel += PixelLayoutBits(i->type);
		
		assert(PixelLayoutBits(i->type) % 8 == 0); // not dealing with weird bits yet
	}
	
	bits_per_pixel += _padding;
	
	assert(bits_per_pixel % 8 == 0);
	
	const size_t pixel_size = (bits_per_pixel >> 3);
	const size_t rowbytes = _descriptor.getWidth() * _channelVec.size() * sizeof(unsigned char);
	const size_t data_size = rowbytes * _descriptor.getHeight();
	
	assert(data_size == data.Size);
	
	FrameBuffer encoded_frameBuffer(_descriptor.getWidth(), _descriptor.getHeight());
	
	
	FrameBufferPtr exported_frameBuffer = new FrameBuffer(_descriptor.getWidth(), _descriptor.getHeight());
	
	DataChunkPtr data_to_store = new DataChunk(data_size);
	
	exported_frameBuffer->attachData(data_to_store);
	
	
	char *encoded_origin = (char *)data.Data;
	
	char *exported_origin = (char *)data_to_store->Data;
	
	for(int i = 0; i < _channelVec.size(); i++)
	{
		const ChannelBits &chan = _channelVec[i];
		
		const size_t bytes_this_subpixel = (PixelLayoutBits(chan.type) >> 3);
	
		if(chan.code != 'F')
		{
			assert(chan.type == MoxFiles::UINT8);
			
			encoded_frameBuffer.insert(chan.name, Slice(MoxFiles::UINT8, encoded_origin, pixel_size, rowbytes));
			
			encoded_origin += bytes_this_subpixel;
			
			exported_frameBuffer->insert(chan.name, Slice(MoxFiles::UINT8, exported_origin, pixel_size, rowbytes));
			
			exported_origin += bytes_this_subpixel;
		}
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
UncompressedCodecInfo::createCodec(MoxMxf::VideoDescriptor *descriptor, Header &header, ChannelList &channels) const
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
