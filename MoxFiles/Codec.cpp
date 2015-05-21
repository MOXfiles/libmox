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
#include <MoxFiles/MPEGCodec.h>

#include <MoxFiles/UncompressedPCMCodec.h>


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


typedef std::map<VideoCompression, VideoCodecInfo *> VideoCodecList;
	
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
		codecList[MPEG] = new MPEGCodecInfo;
	}
	
	if(codecList.find(videoCompression) == codecList.end())
		throw MoxMxf::ArgExc("Requested VideoCodecInfo for unknown compression.");
	
	return *codecList[videoCompression];
}


const VideoCodecInfo &
getVideoCodecInfo(MoxMxf::VideoDescriptor::VideoCodec codec)
{
	if(codec == MoxMxf::VideoDescriptor::VideoCodecUncompressedRGB)
	{
		return getVideoCodecInfo(UNCOMPRESSED);
	}
	else if(codec == MoxMxf::VideoDescriptor::VideoCodecMPEG2)
	{
		return getVideoCodecInfo(MPEG);
	}
	
	throw MoxMxf::InputExc("Unknown video codec");
}


DataChunkPtr
AudioCodec::getNextData()
{
	DataChunkPtr dat;
	
	if(!_data_queue.empty())
	{
		dat = _data_queue.front();
		
		_data_queue.pop();
	}
	
	return dat;
}


AudioBufferPtr
AudioCodec::getNextBuffer()
{
	AudioBufferPtr buf;
	
	if(!_audio_queue.empty())
	{
		buf = _audio_queue.front();
		
		_audio_queue.pop();
	}
	
	return buf;
}


void
AudioCodec::storeData(DataChunkPtr dat)
{
	_data_queue.push(dat);
}


void
AudioCodec::storeBuffer(AudioBufferPtr buf)
{
	_audio_queue.push(buf);
}


typedef std::map<AudioCompression, AudioCodecInfo *> AudioCodecList;
	
class AudioCodecListStorage
{
  public:
	AudioCodecListStorage() {}
	~AudioCodecListStorage();
	
	AudioCodecList & getCodecList() { return _audio_codec_list; }

  private:
	AudioCodecList _audio_codec_list;
};

AudioCodecListStorage::~AudioCodecListStorage()
{
	for(AudioCodecList::iterator i = _audio_codec_list.begin(); i != _audio_codec_list.end(); ++i)
	{
		delete i->second;
	}
}


const AudioCodecInfo &
getAudioCodecInfo(AudioCompression audioCompression)
{
	static AudioCodecListStorage codecListStorage;
	
	AudioCodecList &codecList = codecListStorage.getCodecList();
	
	if( codecList.empty() )
	{
		codecList[PCM] = new UncompressedPCMCodecInfo;
	}
	
	if(codecList.find(audioCompression) == codecList.end())
		throw MoxMxf::ArgExc("Requested AudioCodecInfo for unknown compression.");
	
	return *codecList[audioCompression];
}


const AudioCodecInfo &
getAudioCodecInfo(MoxMxf::AudioDescriptor::AudioCodec codec)
{
	if(codec == MoxMxf::AudioDescriptor::AudioCodecUncompressedPCM ||
		codec == MoxMxf::AudioDescriptor::AudioCodecAES3)
	{
		return getAudioCodecInfo(PCM);
	}
	
	throw MoxMxf::InputExc("Unknown video codec");
}


} // namespace
