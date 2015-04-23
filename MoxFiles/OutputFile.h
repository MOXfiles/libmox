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

#include <MoxMxf/OutputFile.h>


namespace MoxFiles
{
	class OutputFile
	{
	  public:
		OutputFile(IOStream &outfile, const Header &header);
		~OutputFile();
		
		void pushFrame(const FrameBufferPtr frame);
	  
	  private:
		Header _header;
		MoxMxf::OutputFile *_mxf_file;
		
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


#endif