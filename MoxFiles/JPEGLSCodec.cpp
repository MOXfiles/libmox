/*
 *  JPEGLSCodec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 11/26/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#include <MoxFiles/JPEGLSCodec.h>

#include <MoxFiles/MemoryFile.h>

//#include <iostream>

#include <assert.h>

#undef ASSERT // defined in mxflib

#include "interface.h"

namespace MoxFiles
{

JPEGLSCodec::JPEGLSCodec(const Header &header, const ChannelList &channels) :
	VideoCodec(header, channels),
	_descriptor(header.frameRate(), header.width(), header.height(), MoxMxf::VideoDescriptor::VideoCodecJPEGLS)
{
	setWindows(_descriptor, header);
	
	const Channel *r_channel = channels.findChannel("R");
	const Channel *a_channel = channels.findChannel("A");
	
	if(r_channel == NULL)
		throw MoxMxf::ArgExc("Expected RGB(A) channels");
	
	
	_channels = (a_channel != NULL ? JPEGLS_RGBA : JPEGLS_RGB);
	
	
	const unsigned int bit_depth = PixelBits(r_channel->type);
	
	_depth = (bit_depth == 8 ? JPEGLS_8 :
				bit_depth == 10 ? JPEGLS_10 :
				bit_depth == 12 ? JPEGLS_12 :
				bit_depth == 16 ? JPEGLS_16 :
				JPEGLS_8);
	
	
	MoxMxf::RGBADescriptor::RGBALayout layout;
	
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('R', bit_depth));
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('G', bit_depth));
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('B', bit_depth));
	
	if(_channels == JPEGLS_RGBA)
		layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('A', bit_depth));
	
	_descriptor.setPixelLayout(layout);
}


JPEGLSCodec::JPEGLSCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<const MoxMxf::RGBADescriptor &>(descriptor))
{
	assert(_descriptor.getVideoCodec() == MoxMxf::VideoDescriptor::VideoCodecJPEGLS);

	const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = _descriptor.getPixelLayout();
	
	for(MoxMxf::RGBADescriptor::RGBALayout::const_iterator i = pixelLayout.begin(); i != pixelLayout.end(); ++i)
	{
		const MoxMxf::RGBADescriptor::RGBALayoutItem &item = *i;
		
		if(item.code == 'A')
			_channels = JPEGLS_RGBA;
		
		if(item.depth == 8)
			_depth = JPEGLS_8;
		else if(item.depth == 10)
			_depth = JPEGLS_10;
		else if(item.depth == 12)
			_depth = JPEGLS_12;
		else if(item.depth == 16)
			_depth = JPEGLS_16;
		else
			throw MoxMxf::InputExc("Unexpected bit depth");
	}
	
	
	const PixelType pixel_type = (_depth == JPEGLS_8 ? UINT8 :
									_depth == JPEGLS_10 ? UINT10 :
									_depth == JPEGLS_12 ? UINT12 :
									_depth == JPEGLS_16 ? UINT16 :
									UINT8);
									
	channels.insert("R", Channel(pixel_type));
	channels.insert("G", Channel(pixel_type));
	channels.insert("B", Channel(pixel_type));
	
	if(_channels == JPEGLS_RGBA)
		channels.insert("A", Channel(pixel_type));
}


JPEGLSCodec::~JPEGLSCodec()
{

}


void
JPEGLSCodec::compress(const FrameBuffer &frame)
{
	const Box2i dataW = dataWindow();
	
	const int width = (dataW.max.x - dataW.min.x + 1);
	const int height = (dataW.max.y - dataW.min.y + 1);

	const PixelType pixType = (_depth == JPEGLS_8 ? UINT8 :
								_depth == JPEGLS_10 ? UINT10 :
								_depth == JPEGLS_12 ? UINT12 :
								_depth == JPEGLS_16 ? UINT16 :
								UINT8);
	
	const size_t pixSize = PixelSize(pixType);
	const unsigned int bitDepth = PixelBits(pixType);
	const int numChannels = (_channels == JPEGLS_RGBA ? 4 : 3);
	
	const size_t tempPixelSize = (numChannels * pixSize);
	const size_t tempRowbytes = (tempPixelSize * width);
	const size_t tempBufSize = (tempRowbytes * height);
	
	DataChunk dataChunk(tempBufSize);
	
	char *tempBuffer = (char *)dataChunk.Data;
	
	assert(dataW.min.x == 0 && dataW.min.y == 0);
	
	FrameBuffer tempFrameBuffer(dataW);
	
	tempFrameBuffer.insert("R", Slice(pixType, &tempBuffer[0 * pixSize], tempPixelSize, tempRowbytes));
	tempFrameBuffer.insert("G", Slice(pixType, &tempBuffer[1 * pixSize], tempPixelSize, tempRowbytes));
	tempFrameBuffer.insert("B", Slice(pixType, &tempBuffer[2 * pixSize], tempPixelSize, tempRowbytes));
	
	if(_channels == JPEGLS_RGBA)
		tempFrameBuffer.insert("A", Slice(pixType, &tempBuffer[3 * pixSize], tempPixelSize, tempRowbytes));
	
	tempFrameBuffer.copyFromFrame(frame);
	
	
	JlsParameters params = JlsParameters();
	
	params.width = width;
	params.height = height;
	params.bitspersample = bitDepth;
	//params.bytesperline = (tempRowbytes / 3);
	params.components = numChannels;
	params.allowedlossyerror = 0; // always lossless
	params.ilv = ILV_SAMPLE;
	params.colorTransform = COLORXFORM_NONE;
	//params.outputBgr = 0;
	
	/*
	params.custom.MAXVAL = 255;
	params.custom.T1 = 0;
	params.custom.T2 = 0;
	params.custom.T3 = 0;
	params.custom.RESET = 1;
	
	
	params.jfif.Ver = 123;
	params.jfif.units = 0;
	params.jfif.XDensity = 72;
	params.jfif.YDensity = 72;
	params.jfif.Xthumb = 0;
	params.jfif.Ythumb = 0;
	params.jfif.pdataThumbnail = NULL;
	*/
	
	
	ByteStreamInfo inStream = FromByteArray(dataChunk.Data, dataChunk.Size);
	
	DataChunkPtr outDataChunk = new DataChunk(tempBufSize);
	
	
	size_t bytesWritten = 0;
	
	JLS_ERROR err = OK;
	
	do
	{
		ByteStreamInfo outStream = FromByteArray(outDataChunk->Data, outDataChunk->Size);
		
		err = JpegLsEncodeStream(outStream, &bytesWritten, inStream, &params);
		
		if(err == CompressedBufferTooSmall)
		{
			outDataChunk->Resize(2 * outDataChunk->Size, false);
		}
	
	}while(err == CompressedBufferTooSmall);
	
	assert(err != TooMuchCompressedData);
	
	
	if(err == OK)
	{
		assert(bytesWritten > 0);
	
		outDataChunk->Resize(bytesWritten);
		
		storeData(outDataChunk);
	}
	else
		throw MoxMxf::ArgExc("JPEG-LS compression error");
}


void
JPEGLSCodec::decompress(const DataChunk &data)
{
	ByteStreamInfo inStream = FromByteArray(data.Data, data.Size);
	
	
	struct JlsParameters info;
	
	JLS_ERROR err = JpegLsReadHeaderStream(inStream, &info);
	
	if(err == OK)
	{
		const int width = info.width;
		const int height = info.height;
		
		assert(info.components == (_channels == JPEGLS_RGBA ? 4 : 3));
		assert(info.colorTransform == COLORXFORM_NONE);
		
		const PixelType pixType = (_depth == JPEGLS_8 ? UINT8 :
									_depth == JPEGLS_10 ? UINT10 :
									_depth == JPEGLS_12 ? UINT12 :
									_depth == JPEGLS_16 ? UINT16 :
									UINT8);
		
		const size_t pixSize = PixelSize(pixType);
		const unsigned int bitDepth = PixelBits(pixType);
		
		assert(info.bitspersample == bitDepth);
									
		const size_t pixelSize = (info.components * PixelSize(pixType));
		const size_t rowBytes = (width * pixelSize);
		const size_t bufSize = (height * rowBytes);
		
		DataChunkPtr frameData = new DataChunk(bufSize);
		
		char *buf = (char *)frameData->Data;
		
		const Box2i dataW = dataWindow();
		
		assert(width == (dataW.max.x - dataW.min.x + 1));
		assert(height == (dataW.max.y - dataW.min.y + 1));
		assert(dataW.min.x == 0);
		assert(dataW.min.y == 0);
			
		FrameBufferPtr frameBuffer = new FrameBuffer(dataW);
		
		frameBuffer->insert("R", Slice(pixType, &buf[0 * pixSize], pixelSize, rowBytes));
		frameBuffer->insert("G", Slice(pixType, &buf[1 * pixSize], pixelSize, rowBytes));
		frameBuffer->insert("B", Slice(pixType, &buf[2 * pixSize], pixelSize, rowBytes));
		
		if(info.components >= 4)
			frameBuffer->insert("A", Slice(pixType, &buf[3 * pixSize], pixelSize, rowBytes));
		
		frameBuffer->attachData(frameData);
		
		
		ByteStreamInfo outStream = FromByteArray(frameData->Data, frameData->Size);
		
		
		err = JpegLsDecodeStream(outStream, inStream, &info);
		
		if(err == OK)
		{
			storeFrame(frameBuffer);
		}
	}
	
	if(err != OK)
		throw MoxMxf::ArgExc("JPEG-LS decompression error");
}


bool
JPEGLSCodecInfo::canCompressType(PixelType pixelType) const
{
	return (pixelType == UINT8 || pixelType == UINT10 || pixelType == UINT12);
}


ChannelCapabilities
JPEGLSCodecInfo::getChannelCapabilites() const
{
	return (Channels_RGB | Channels_RGBA);
}


VideoCodec * 
JPEGLSCodecInfo::createCodec(const Header &header, const ChannelList &channels) const
{
	return new JPEGLSCodec(header, channels);
}

VideoCodec * 
JPEGLSCodecInfo::createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const
{
	return new JPEGLSCodec(descriptor, header, channels);
}


} // namespace
