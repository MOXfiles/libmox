/*
 *  Codec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/22/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_CODEC_H
#define MOXFILES_CODEC_H

#include <MoxFiles/Header.h>
//#include <MoxFiles/Types.h>
#include <MoxFiles/FrameBuffer.h>

#include <MoxMxf/Descriptor.h>

#include <queue>

namespace MoxFiles
{
	class VideoCodec
	{
	  public:
		VideoCodec(const Header &header, const ChannelList &channels) {} // for compression
		VideoCodec(MoxMxf::VideoDescriptor *descriptor, Header &header, const ChannelList &channels) {} // for decompression
		virtual ~VideoCodec() {}
		
		virtual MoxMxf::Descriptor * getDescriptor() = 0;
		
		virtual void compress(FrameBufferPtr frame) = 0;
		virtual DataChunkPtr getNextData();
		
		virtual void decompress(const DataChunk &data) = 0;
		virtual FrameBufferPtr getNextFrame();
	
	  protected:
		virtual void storeData(DataChunkPtr dat);
		virtual void storeFrame(FrameBufferPtr frm);
		
	  private:
		std::queue<DataChunkPtr> _data_queue;
		std::queue<FrameBufferPtr> _frame_queue;
	};
	

	enum Channels
	{
		Channels_None	= 0,
		Channels_RGB	= 1,
		Channels_RGBA	= 2,
		Channels_A		= 4,
		Channels_Any	= 8,
		
		Channels_All	= Channels_RGB | Channels_RGBA | Channels_A | Channels_Any
	};
	
	typedef int ChannelCapabilities;
	
	
	class VideoCodecInfo
	{
	  public:
		VideoCodecInfo() {}
		virtual ~VideoCodecInfo() {}
		
		//virtual bool canCompressType(PixelType pixelType) const = 0;
		
		
		virtual ChannelCapabilities getChannelCapabilites() const = 0;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const = 0; // compression
		virtual VideoCodec * createCodec(MoxMxf::VideoDescriptor *descriptor, Header &header, const ChannelList &channels) const = 0; // decompression
	};
	

	typedef std::map<VideoCompression, VideoCodecInfo *> VideoCodecList;
	
	const VideoCodecInfo & getVideoCodecInfo(VideoCompression videoCompression);
	//const VideoCodecInfo & getVideoCodecInfo(MoxMxf::VideoDescriptor::VideoCodec videoCodec);
}

#endif // MOXFILES_CODEC_H
