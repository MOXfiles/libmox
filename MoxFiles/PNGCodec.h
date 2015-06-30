/*
 *  PNGCodec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 6/24/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_PNGCODEC_H
#define MOXFILES_PNGCODEC_H

#include <MoxFiles/Codec.h>

namespace MoxFiles
{
	class PNGCodec : public VideoCodec
	{
	  public:
		PNGCodec(const Header &header, const ChannelList &channels);
		PNGCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels);
		virtual ~PNGCodec();
		
		virtual const MoxMxf::VideoDescriptor * getDescriptor() const { return &_descriptor; }
		
		virtual void compress(const FrameBuffer &frame);
		virtual void decompress(const DataChunk &data);
		
	  private:
		MoxMxf::RGBADescriptor _descriptor;
		
		enum PNG_Channels {
			PNG_RGB,
			PNG_RGBA,
			PNG_Y,
			PNG_YA
		};
		
		PNG_Channels _channels;
		
		enum PNG_Depth {
			PNG_8,
			PNG_16
		};
		
		PNG_Depth _depth;
	};
	
	
	class PNGCodecInfo : public VideoCodecInfo
	{
	  public:
		PNGCodecInfo() {}
		virtual ~PNGCodecInfo() {}
		
		virtual bool canCompressType(PixelType pixelType) const;
		
		virtual ChannelCapabilities getChannelCapabilites() const;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const;
		virtual VideoCodec * createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const;
	};

} // namespace

#endif