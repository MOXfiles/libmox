/*
 *  JPEGLSCodec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 11/26/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_JPEGLSCODEC_H
#define MOXFILES_JPEGLSCODEC_H

#include <MoxFiles/Codec.h>

namespace MoxFiles
{
	class JPEGLSCodec : public VideoCodec
	{
	  public:
		JPEGLSCodec(const Header &header, const ChannelList &channels);
		JPEGLSCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels);
		virtual ~JPEGLSCodec();
		
		virtual const MoxMxf::VideoDescriptor * getDescriptor() const { return &_descriptor; }
		
		virtual void compress(const FrameBuffer &frame);
		virtual void decompress(const DataChunk &data);
	
	  private:
		MoxMxf::RGBADescriptor _descriptor;
		
		enum JPEGLS_Channels {
			JPEGLS_RGB,
			JPEGLS_RGBA
		};
		
		JPEGLS_Channels _channels;
		
		enum JPEGLS_Depth {
			JPEGLS_8,
			JPEGLS_10,
			JPEGLS_12,
			JPEGLS_16
		};
		
		JPEGLS_Depth _depth;
	};
	
	
	class JPEGLSCodecInfo : public VideoCodecInfo
	{
	  public:
		JPEGLSCodecInfo() {}
		virtual ~JPEGLSCodecInfo() {}
		
		virtual bool canCompressType(PixelType pixelType) const;
		
		virtual ChannelCapabilities getChannelCapabilites() const;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const;
		virtual VideoCodec * createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const;
	};

} // namespace

#endif // MOXFILES_JPEGLSCODEC_H
