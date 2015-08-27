/*
 *  MPEGCodec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 5/12/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/MPEGCodec.h>


namespace MoxFiles
{

MPEGCodec::MPEGCodec(const Header &header, const ChannelList &channels) :
	VideoCodec(header, channels),
	_descriptor(header.frameRate(), header.width(), header.height())
{
	throw MoxMxf::NoImplExc("Implement!");
}


MPEGCodec::MPEGCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<const MoxMxf::MPEGDescriptor &>(descriptor))
{
	assert(header.width() == _descriptor.getStoredWidth());
	assert(header.height() == _descriptor.getStoredHeight());
	
	channels.insert("R", Channel(MoxFiles::UINT8));
	channels.insert("G", Channel(MoxFiles::UINT8));
	channels.insert("B", Channel(MoxFiles::UINT8));
}


MPEGCodec::~MPEGCodec()
{

}


void
MPEGCodec::compress(const FrameBuffer &frame)
{
	throw MoxMxf::NoImplExc("This is a fake codec");
}


void
MPEGCodec::decompress(const DataChunk &data)
{
	const int channels = 3;
	const size_t bytes_per_channel = sizeof(unsigned char);
	
	const UInt32 width = _descriptor.getStoredWidth();
	const UInt32 height = _descriptor.getStoredHeight();
	
	const ptrdiff_t stride = bytes_per_channel * channels;
	const size_t rowbytes = width * stride;
	const size_t data_size = rowbytes * height;
	
	DataChunkPtr buf_data = new DataChunk(data_size);
	
	char *data_origin = (char *)buf_data->Data;
	
	
	
	FrameBufferPtr buf = new FrameBuffer(width, height);
	
	buf->insert("R", Slice(MoxFiles::UINT8, data_origin + (bytes_per_channel * 0), stride, rowbytes));
	buf->insert("G", Slice(MoxFiles::UINT8, data_origin + (bytes_per_channel * 1), stride, rowbytes));
	buf->insert("B", Slice(MoxFiles::UINT8, data_origin + (bytes_per_channel * 2), stride, rowbytes));
	
	memset(data_origin, 0, data_size);
	
	buf->attachData(buf_data);
	
	
	storeFrame(buf);
}

bool
MPEGCodecInfo::canCompressType(PixelType pixelType) const
{
	return (pixelType == UINT8);
}

ChannelCapabilities
MPEGCodecInfo::getChannelCapabilites() const
{
	return Channels_RGB;
}


VideoCodec *
MPEGCodecInfo::createCodec(const Header &header, const ChannelList &channels) const
{
	return new MPEGCodec(header, channels);
}


VideoCodec *
MPEGCodecInfo::createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const
{
	return new MPEGCodec(descriptor, header, channels);
}


} // namespace
