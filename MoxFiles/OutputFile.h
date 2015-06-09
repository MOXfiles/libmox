/*
 *  OutputFile.h
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_OUTPUTFILE_H
#define MOXFILES_OUTPUTFILE_H

#include <MoxFiles/Header.h>

#include <MoxFiles/Codec.h>
#include <MoxFiles/FrameBuffer.h>
#include <MoxFiles/AudioBuffer.h>

#include <MoxMxf/OutputFile.h>


namespace MoxFiles
{
	class OutputFile
	{
	  public:
		OutputFile(IOStream &outfile, const Header &header);
		~OutputFile();
		
		void pushFrame(const FrameBuffer &frame);
		
		void pushAudio(const AudioBuffer &audio);
	  
	  private:
		Header _header;
		MoxMxf::OutputFile *_mxf_file;
		
		typedef struct VideoCodecUnit
		{
			ChannelList channelList;
			VideoCodec *codec;
			MoxMxf::TrackNum trackNumber;
			
			VideoCodecUnit() : codec(NULL) {}
			VideoCodecUnit(ChannelList ch, VideoCodec *co, MoxMxf::TrackNum tr) : channelList(ch), codec(co), trackNumber(tr) {}
		} VideoCodecUnit;
		
		std::list<VideoCodecUnit> _video_codec_units;
		
		int _video_frames;
		
		
		typedef struct AudioCodecUnit
		{
			AudioChannelList channelList;
			AudioCodec *codec;
			MoxMxf::TrackNum trackNumber;
			
			AudioBufferPtr audioBuffer;
			
			AudioCodecUnit() : codec(NULL) {}
			AudioCodecUnit(AudioChannelList ch, AudioCodec *co, MoxMxf::TrackNum tr) : channelList(ch), codec(co), trackNumber(tr) {}
		} AudioCodecUnit;
		
		std::list<AudioCodecUnit> _audio_codec_units;
		
		int _audio_frames;
	};

} // namespace


#endif