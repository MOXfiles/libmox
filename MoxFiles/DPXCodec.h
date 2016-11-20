/*
 *  DPXCodec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 5/12/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_DPXCODEC_H
#define MOXFILES_DPXCODEC_H

#include <MoxFiles/Codec.h>

namespace MoxFiles
{
	class DPXCodec : public VideoCodec
	{
	  public:
		DPXCodec(const Header &header, const ChannelList &channels);
		DPXCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels);
		virtual ~DPXCodec();
		
		virtual const MoxMxf::VideoDescriptor * getDescriptor() const { return &_descriptor; }
		
		virtual void compress(const FrameBuffer &frame);
		virtual void decompress(const DataChunk &data);
	
	  private:
		MoxMxf::RGBADescriptor _descriptor;
		
		enum DPX_Channels {
			DPX_RGB,
			DPX_RGBA,
		};
		
		DPX_Channels _channels;
		
		enum DPX_Depth {
			DPX_8,
			DPX_10,
			DPX_12,
			DPX_16
		};
		
		DPX_Depth _depth;
		
		const Rational _pixelAspectRatio;
		const Rational _frameRate;
	};
	
	
	class DPXCodecInfo : public VideoCodecInfo
	{
	  public:
		DPXCodecInfo() {}
		virtual ~DPXCodecInfo() {}
		
		virtual bool canCompressType(PixelType pixelType) const;
		
		virtual ChannelCapabilities getChannelCapabilites() const;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const;
		virtual VideoCodec * createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const;
	};
	
} // namespace

#endif // MOXFILES_DPXCODEC_H
