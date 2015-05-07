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
		
	  private:
		MoxMxf::InputFile _mxf_file;
		MoxMxf::SID _bodySID;
		MoxMxf::SID _indexSID;
		
		Header _header;
		
		typedef struct CodecUnit
		{
			ChannelList channelList;
			VideoCodec *codec;
			MoxMxf::TrackNum trackNumber;
			
			CodecUnit() : codec(NULL) {}
			CodecUnit(ChannelList ch, VideoCodec *co, MoxMxf::TrackNum tr) : channelList(ch), codec(co), trackNumber(tr) {}
		} CodecUnit;
		
		std::list<CodecUnit> _codec_units;
	};

} // namespace

#endif // MOXFILES_INPUTFILE_H