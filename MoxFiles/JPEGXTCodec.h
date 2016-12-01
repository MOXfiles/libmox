/*
 *  JPEGXTCodec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 11/27/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_JPEGXTCODEC_H
#define MOXFILES_JPEGXTCODEC_H

#include <MoxFiles/Codec.h>

namespace MoxFiles
{
	class JPEGXTCodec : public VideoCodec
	{
	  public:
		JPEGXTCodec(const Header &header, const ChannelList &channels);
		JPEGXTCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels);
		virtual ~JPEGXTCodec();
		
		virtual const MoxMxf::VideoDescriptor * getDescriptor() const { return &_descriptor; }
		
		virtual void compress(const FrameBuffer &frame);
		virtual void decompress(const DataChunk &data);
	
	  private:
		MoxMxf::RGBADescriptor _descriptor;
		
		
		enum JPEGXT_Channels {
			JPEGXT_RGB,
			JPEGXT_RGBA
		};
		
		JPEGXT_Channels _channels;
		
		enum JPEGXT_Depth {
			JPEGXT_8,
			JPEGXT_10,
			JPEGXT_12,
			JPEGXT_16,
			JPEGXT_FLOAT
		};
		
		JPEGXT_Depth _depth;
		
		
		bool _lossless;
		int _quality;
	};
	
	
	class JPEGXTCodecInfo : public VideoCodecInfo
	{
	  public:
		JPEGXTCodecInfo() {}
		virtual ~JPEGXTCodecInfo() {}
		
		virtual bool canCompressType(PixelType pixelType) const;
		
		virtual ChannelCapabilities getChannelCapabilites() const;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const;
		virtual VideoCodec * createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const;
	};

} // namespace

#endif // MOXFILES_JPEGXTCODEC_H
