/*
 *  MPEGCodec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 5/12/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_MPEGCODEC_H
#define MOXFILES_MPEGCODEC_H

#include <MoxFiles/Codec.h>

namespace MoxFiles
{
	class MPEGCodec : public VideoCodec
	{
	  public:
		MPEGCodec(const Header &header, const ChannelList &channels);
		MPEGCodec(MoxMxf::VideoDescriptor *descriptor, Header &header, ChannelList &channels);
		virtual ~MPEGCodec();
		
		virtual MoxMxf::VideoDescriptor * getDescriptor() { return &_descriptor; }
		
		virtual void compress(FrameBufferPtr frame);
		virtual void decompress(const DataChunk &data);
		
	  private:
		MoxMxf::MPEGDescriptor _descriptor;
	};
	
	
	class MPEGCodecInfo : public VideoCodecInfo
	{
	  public:
		MPEGCodecInfo() {}
		virtual ~MPEGCodecInfo() {}
		
		virtual ChannelCapabilities getChannelCapabilites() const;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const;
		virtual VideoCodec * createCodec(MoxMxf::VideoDescriptor *descriptor, Header &header, ChannelList &channels) const;
	};

} // namespace

#endif // MOXFILES_MPEGCODEC_H