/*
 *  UncompressedCodec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/22/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_UNCOMPRESSEDCODEC_H
#define MOXFILES_UNCOMPRESSEDCODEC_H

#include <MoxFiles/Codec.h>

namespace MoxFiles
{
	class UncompressedCodec : public VideoCodec
	{
	  public:
		UncompressedCodec(const Header &header, const ChannelList &channels);
		virtual ~UncompressedCodec();
		
		virtual MoxMxf::Descriptor * getDescriptor() { return &_descriptor; }
				
		virtual DataChunkPtr compress(FrameBufferPtr frame);
		virtual FrameBufferPtr decompress(DataChunkPtr data);
	
	  private:
		MoxMxf::RGBADescriptor _descriptor;
	};
	
	class UncompressedCodecInfo : public VideoCodecInfo
	{
	  public:
		UncompressedCodecInfo() {}
		virtual ~UncompressedCodecInfo() {}
		
		//virtual bool canCompressType(PixelType pixelType) const;
		
		virtual ChannelCapabilities getChannelCapabilites() const;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const;
	};

} // namepsace

#endif