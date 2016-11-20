/*
 *  JPEG2000Codec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 11/15/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_JPEG2000CODEC_H
#define MOXFILES_JPEG2000CODEC_H

#include <MoxFiles/Codec.h>

namespace MoxFiles
{
	class JPEG2000Codec : public VideoCodec
	{
	  public:
		JPEG2000Codec(const Header &header, const ChannelList &channels);
		JPEG2000Codec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels);
		virtual ~JPEG2000Codec();
		
		virtual const MoxMxf::VideoDescriptor * getDescriptor() const { return &_descriptor; }
		
		virtual void compress(const FrameBuffer &frame);
		virtual void decompress(const DataChunk &data);
	
	  private:
		MoxMxf::RGBADescriptor _descriptor;
		
		enum JP2_Channels {
			JP2_RGB,
			JP2_RGBA,
		};
		
		JP2_Channels _channels;
		
		enum JP2_Depth {
			JP2_8,
			JP2_10,
			JP2_12,
			JP2_16
		};
		
		JP2_Depth _depth;
		
		bool _lossless;
		int _quality;
	};
	
	
	class JPEG2000CodecInfo : public VideoCodecInfo
	{
	  public:
		JPEG2000CodecInfo() {}
		virtual ~JPEG2000CodecInfo() {}
		
		virtual bool canCompressType(PixelType pixelType) const;
		
		virtual ChannelCapabilities getChannelCapabilites() const;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const;
		virtual VideoCodec * createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const;
	};

} // namespace


#endif // MOXFILES_JPEG2000CODEC_H
