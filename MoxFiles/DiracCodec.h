/*
 *  DiracCodec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 7/28/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_DIRACCODEC_H
#define MOXFILES_DIRACCODEC_H

#include <MoxFiles/Codec.h>

#include <schroedinger/schro.h>

namespace MoxFiles
{
	class DiracCodec : public VideoCodec
	{
	  public:
		DiracCodec(const Header &header, const ChannelList &channels);
		DiracCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels);
		virtual ~DiracCodec();
		
		virtual const MoxMxf::VideoDescriptor * getDescriptor() const { return _descriptor; }
		
		virtual void compress(const FrameBuffer &frame);
		virtual void decompress(const DataChunk &data);
		
		virtual void end_of_stream();
		
	  private:
		MoxMxf::VideoDescriptor *_descriptor;
		
		SchroEncoder *_encoder;
		SchroDecoder *_decoder;
		
		void encoder_pull();
		void decoder_pull();
	};
	
	
	class DiracCodecInfo : public VideoCodecInfo
	{
	  public:
		DiracCodecInfo() {}
		virtual ~DiracCodecInfo() {}
		
		virtual bool canCompressType(PixelType pixelType) const;
		
		virtual ChannelCapabilities getChannelCapabilites() const;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const;
		virtual VideoCodec * createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const;
	};

} // namespace

#endif // MOXFILES_DIRACCODEC_H
