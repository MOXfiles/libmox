/*
 *  OpenEXRCodec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 6/29/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_OPENEXRCODEC_H
#define MOXFILES_OPENEXRCODEC_H

#include <MoxFiles/Codec.h>

#include <ImfHeader.h>

namespace MoxFiles
{
	class OpenEXRCodec : public VideoCodec
	{
	  public:
		OpenEXRCodec(const Header &header, const ChannelList &channels);
		OpenEXRCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels);
		virtual ~OpenEXRCodec();
		
		virtual const MoxMxf::VideoDescriptor * getDescriptor() const { return &_descriptor; }
		
		virtual void compress(const FrameBuffer &frame);
		virtual void decompress(const DataChunk &data);
		
	  private:
		MoxMxf::RGBADescriptor _descriptor;
		
		Imf::Header *_exr_header;
	};
	
	
	class OpenEXRCodecInfo : public VideoCodecInfo
	{
	  public:
		OpenEXRCodecInfo() {}
		virtual ~OpenEXRCodecInfo() {}
		
		virtual bool canCompressType(PixelType pixelType) const;
		
		virtual ChannelCapabilities getChannelCapabilites() const;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const;
		virtual VideoCodec * createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const;
	};	

} // namespace

#endif // MOXFILES_OPENEXRCODEC_H
