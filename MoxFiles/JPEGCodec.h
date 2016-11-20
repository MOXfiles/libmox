/*
 *  JPEGCodec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 11/15/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_JPEGCODEC_H
#define MOXFILES_JPEGCODEC_H

#include <MoxFiles/Codec.h>

namespace MoxFiles
{
	class JPEGCodec : public VideoCodec
	{
	  public:
		JPEGCodec(const Header &header, const ChannelList &channels);
		JPEGCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels);
		virtual ~JPEGCodec();
		
		virtual const MoxMxf::VideoDescriptor * getDescriptor() const { return &_descriptor; }
		
		virtual void compress(const FrameBuffer &frame);
		virtual void decompress(const DataChunk &data);
	
	  private:
		MoxMxf::RGBADescriptor _descriptor;
		
		int _quality;
	};
	
	
	class JPEGCodecInfo : public VideoCodecInfo
	{
	  public:
		JPEGCodecInfo() {}
		virtual ~JPEGCodecInfo() {}
		
		virtual bool canCompressType(PixelType pixelType) const;
		
		virtual ChannelCapabilities getChannelCapabilites() const;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const;
		virtual VideoCodec * createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const;
	};

} // namespace

#endif // MOXFILES_JPEGCODEC_H
