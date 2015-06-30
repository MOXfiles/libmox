/*
 *  UncompressedCodec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/22/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_UNCOMPRESSEDVIDEOCODEC_H
#define MOXFILES_UNCOMPRESSEDVIDEOCODEC_H

#include <MoxFiles/Codec.h>

namespace MoxFiles
{
	class UncompressedVideoCodec : public VideoCodec
	{
	  public:
		UncompressedVideoCodec(const Header &header, const ChannelList &channels);
		UncompressedVideoCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels);
		virtual ~UncompressedVideoCodec();
		
		virtual const MoxMxf::VideoDescriptor * getDescriptor() const { return &_descriptor; }
				
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
		
		friend class CompressChannelBits;
		friend class DecompressChannelBits;
		
		std::vector<ChannelBits> _channelVec;
		unsigned char _padding;
	};
	
	class UncompressedVideoCodecInfo : public VideoCodecInfo
	{
	  public:
		UncompressedVideoCodecInfo() {}
		virtual ~UncompressedVideoCodecInfo() {}
		
		virtual bool canCompressType(PixelType pixelType) const;
		
		virtual ChannelCapabilities getChannelCapabilites() const;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const;
		virtual VideoCodec * createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const;
	};

} // namepsace

#endif // MOXFILES_UNCOMPRESSEDVIDEOCODEC_H
