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

UncompressedCodec::UncompressedCodec(const Header &header, const ChannelList &channels) :
	VideoCodec(header, channels),
	_descriptor(header.frameRate(), header.width(), header.height())
{

}


UncompressedCodec::~UncompressedCodec()
{

}


DataChunkPtr
UncompressedCodec::compress(FrameBufferPtr frame)
{
	DataChunkPtr data;
	
	return data;
}


FrameBufferPtr
UncompressedCodec::decompress(DataChunkPtr data)
{
	FrameBufferPtr frame;
	
	return frame;
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


//bool
//UncompressedCodecInfo::canCompressType(PixelType pixelType) const
//{
//	// SMPTE 377M E.2.46
//	// Can store anything but 16-bit float
//	return (pixelType != HALF);
//}

} // namespace
