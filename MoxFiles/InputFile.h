/*
 *  InputFile.h
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_INPUTFILE_H
#define MOXFILES_INPUTFILE_H

#include <MoxFiles/Header.h>

#include <MoxFiles/Codec.h>

#include <MoxMxf/InputFile.h>


namespace MoxFiles
{
	class InputFile
	{
	  public:
		InputFile(MoxMxf::IOStream &infile);
		virtual ~InputFile();
		
		const Header & header() const { return _header; }
		
		void getFrame(int frameNumber, FrameBuffer &frameBuffer);
		
		void seekAudio(UInt64 sampleNum) { _sample_num = sampleNum; }
		void readAudio(UInt64 samples, AudioBuffer &buffer);
		
	  private:
		MoxMxf::InputFile _mxf_file;
		MoxMxf::SID _bodySID;
		MoxMxf::SID _indexSID;
		
		Header _header;
		
		typedef struct VideoCodecUnit
		{
			ChannelList channelList;
			VideoCodec *codec;
			MoxMxf::TrackNum trackNumber;
			
			VideoCodecUnit() : codec(NULL) {}
			VideoCodecUnit(ChannelList ch, VideoCodec *co, MoxMxf::TrackNum tr) : channelList(ch), codec(co), trackNumber(tr) {}
		} VideoCodecUnit;
		
		std::list<VideoCodecUnit> _video_codec_units;
		
		
		typedef struct AudioCodecUnit
		{
			AudioChannelList channelList;
			std::map<Name, Name> channel_map;
			AudioCodec *codec;
			MoxMxf::TrackNum trackNumber;
			
			AudioCodecUnit() : codec(NULL) {}
			AudioCodecUnit(AudioChannelList ch, AudioCodec *co, MoxMxf::TrackNum tr) : channelList(ch), codec(co), trackNumber(tr) {}
		
		} AudioCodecUnit;
		
		std::list<AudioCodecUnit> _audio_codec_units;
		
		
		UInt64 _sample_num;
	};

} // namespace

#endif // MOXFILES_INPUTFILE_H