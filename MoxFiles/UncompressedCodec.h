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
		UncompressedCodec(MoxMxf::VideoDescriptor *descriptor, Header &header, ChannelList &channels);
		virtual ~UncompressedCodec();
		
		virtual MoxMxf::VideoDescriptor * getDescriptor() { return &_descriptor; }
				
		virtual void compress(const FrameBuffer &frame);
		virtual void decompress(const DataChunk &data);
	
	  private:
		MoxMxf::RGBADescriptor _descriptor;
		
		typedef struct ChannelBits
		{
			std::string name;
			unsigned char code;
			PixelType type;
			
			ChannelBits(std::string n, unsigned char c, PixelType t) : name(n), code(c), type(t) {}
		} ChannelBits;
		
		std::vector<ChannelBits> _channelVec;
		unsigned char _padding;
	};
	
	class UncompressedCodecInfo : public VideoCodecInfo
	{
	  public:
		UncompressedCodecInfo() {}
		virtual ~UncompressedCodecInfo() {}
		
		//virtual bool canCompressType(PixelType pixelType) const;
		
		virtual ChannelCapabilities getChannelCapabilites() const;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const;
		virtual VideoCodec * createCodec(MoxMxf::VideoDescriptor *descriptor, Header &header, ChannelList &channels) const;
	};

} // namepsace

#endif