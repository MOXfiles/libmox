/*
 *  Codec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/22/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/Codec.h>

#include <MoxFiles/UncompressedCodec.h>

namespace MoxFiles
{

DataChunkPtr
VideoCodec::getNextData()
{
	DataChunkPtr dat;
	
	if(!_data_queue.empty())
	{
		dat = _data_queue.front();
		
		_data_queue.pop();
	}
	
	return dat;
}


FrameBufferPtr
VideoCodec::getNextFrame()
{
	FrameBufferPtr frm;
	
	if(!_frame_queue.empty())
	{
		frm = _frame_queue.front();
		
		_frame_queue.pop();
	}
	
	return frm;
}


void
VideoCodec::storeData(DataChunkPtr dat)
{
	_data_queue.push(dat);
}


void
VideoCodec::storeFrame(FrameBufferPtr frm)
{
	_frame_queue.push(frm);
}


class VideoCodecListStorage
{
  public:
	VideoCodecListStorage() {}
	~VideoCodecListStorage();
	
	VideoCodecList & getCodecList() { return _video_codec_list; }

  private:
	VideoCodecList _video_codec_list;
};

VideoCodecListStorage::~VideoCodecListStorage()
{
	for(VideoCodecList::iterator i = _video_codec_list.begin(); i != _video_codec_list.end(); ++i)
	{
		delete i->second;
	}
}


const VideoCodecInfo &
getVideoCodecInfo(VideoCompression videoCompression)
{
	static VideoCodecListStorage codecListStorage;
	
	VideoCodecList &codecList = codecListStorage.getCodecList();
	
	if( codecList.empty() )
	{
		codecList[UNCOMPRESSED] = new UncompressedCodecInfo;
	}
	
	if(codecList.find(videoCompression) == codecList.end())
		throw MoxMxf::ArgExc("Requested VideoCodecInfo for unknown compression.");
	
	return *codecList[videoCompression];
}

/*
const VideoCodecInfo &
getVideoCodecInfo(MoxMxf::VideoDescriptor::VideoCodec videoCodec)
{
	if(videoCodec == MoxMxf::VideoDescriptor::VideoCodecUncompressedRGB)
	{
		return getVideoCodecInfo(UNCOMPRESSED);
	}
	
	throw MoxMxf::LogicExc("Requested unknown video codec.");
}
*/

} // namespace
