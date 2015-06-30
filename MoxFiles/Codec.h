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
#include <MoxFiles/AudioBuffer.h>

#include <MoxMxf/Descriptor.h>

#include <queue>

namespace MoxFiles
{
	class VideoCodec
	{
	  public:
	  
		// For compression, codec parameters can be put in the header.  We also tell the codec which
		// channels to expect.
		// Unless the codec can handle arbitrary channel names, the ChannelList
		// passed to VideoCodec() should only use the basic channel names like "R", "G", "B".
		// It is up to the caller to pass FrameBuffers that use these names, even if it means
		// copying Slices that use non-basic names.
		//
		// For decompression we set up everything from the descriptor, possibly putting codec
		// parameters back into the non-const header.  The non-const channels object should get
		// the channels in the stream.
		
		VideoCodec(const Header &header, const ChannelList &channels) {} // for compression
		VideoCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) {} // for decompression
		virtual ~VideoCodec() {}
		
		virtual const MoxMxf::VideoDescriptor * getDescriptor() const = 0;
		
		virtual Box2i dataWindow() const;
		virtual Box2i displayWindow() const;
		virtual Box2i sampledWindow() const;
		
		virtual void compress(const FrameBuffer &frame) = 0;
		virtual DataChunkPtr getNextData();
		
		virtual void decompress(const DataChunk &data) = 0;
		virtual FrameBufferPtr getNextFrame();
				
	  protected:		
		virtual void storeData(DataChunkPtr dat);
		virtual void storeFrame(FrameBufferPtr frm);
		
		static void setWindows(MoxMxf::VideoDescriptor &descriptor, const Header &header);
		
	  private:
		std::queue<DataChunkPtr> _data_queue;
		std::queue<FrameBufferPtr> _frame_queue;
	};
	

	enum Channels
	{
		Channels_None	= 0,
		Channels_RGB	= 1L << 0,
		Channels_RGBA	= 1L << 1,
		Channels_Y		= 1L << 2,
		Channels_YA		= 1L << 3,
		Channels_A		= 1L << 4,
		Channels_Any	= 1L << 5, // i.e. a channel with some random name like
		
		Channels_All	= Channels_RGB | Channels_RGBA | Channels_Y | Channels_YA | Channels_A | Channels_Any
	};
	
	typedef UInt32 ChannelCapabilities;
	
	
	class VideoCodecInfo
	{
	  public:
		VideoCodecInfo() {}
		virtual ~VideoCodecInfo() {}
		
		virtual bool canCompressType(PixelType pixelType) const = 0;
		virtual PixelType compressedType(PixelType pixelType) const;
		
		virtual ChannelCapabilities getChannelCapabilites() const = 0;
		
		virtual VideoCodec * createCodec(const Header &header, const ChannelList &channels) const = 0; // compression
		virtual VideoCodec * createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const = 0; // decompression
	};
	
	
	const VideoCodecInfo & getVideoCodecInfo(VideoCompression videoCompression);
	const VideoCodecInfo & getVideoCodecInfo(MoxMxf::VideoDescriptor::VideoCodec videoCodec);
	
	
	
	class AudioCodec
	{
	  public:
		AudioCodec(const Header &header, const AudioChannelList &channels) {} // for compression
		AudioCodec(const MoxMxf::AudioDescriptor &descriptor, Header &header, AudioChannelList &channels) {} // for decompression
		virtual ~AudioCodec() {}
		
		virtual const MoxMxf::AudioDescriptor * getDescriptor() const = 0;
		
		virtual void compress(const AudioBuffer &audio) = 0;
		virtual DataChunkPtr getNextData();
		
		virtual UInt64 samplesInFrame(size_t frame_size) = 0;
		virtual void decompress(const DataChunk &data) = 0;
		virtual AudioBufferPtr getNextBuffer();
		
	  protected:		
		virtual void storeData(DataChunkPtr dat);
		virtual void storeBuffer(AudioBufferPtr audio);
		
	  private:
		std::queue<DataChunkPtr> _data_queue;
		std::queue<AudioBufferPtr> _audio_queue;
	};
	
	
	enum AudioChannels
	{
		AudioChannels_None		= 0,
		AudioChannels_Mono		= 1,
		AudioChannels_Stereo	= 2,
		AudioChannels_51		= 4,
		AudioChannels_Any		= 8,
		
		AudioChannels_All		= AudioChannels_Mono | AudioChannels_Stereo | AudioChannels_51 | AudioChannels_Any
	};
	
	typedef UInt32 AudioChannelCapabilities;
	
	
	class AudioCodecInfo
	{
	  public:
		AudioCodecInfo() {}
		virtual ~AudioCodecInfo() {}
		
		virtual bool canCompressType(SampleType sampleType) const = 0;
		virtual SampleType compressedType(SampleType sampleType) const;
		
		virtual AudioChannelCapabilities getChannelCapabilites() const = 0;
		
		virtual AudioCodec * createCodec(const Header &header, const AudioChannelList &channels) const = 0; // compression
		virtual AudioCodec * createCodec(const MoxMxf::AudioDescriptor &descriptor, Header &header, AudioChannelList &channels) const = 0; // decompression
	};
	
	
	const AudioCodecInfo & getAudioCodecInfo(AudioCompression audioCompression);
	const AudioCodecInfo & getAudioCodecInfo(MoxMxf::AudioDescriptor::AudioCodec audioCodec);
}

#endif // MOXFILES_CODEC_H
