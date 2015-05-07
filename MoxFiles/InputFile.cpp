/*
 *  InputFile.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/InputFile.h>


namespace MoxFiles
{

static const VideoCodecInfo &
getVideoCodecInfo(MoxMxf::VideoDescriptor::VideoCodec codec)
{
	if(codec == MoxMxf::VideoDescriptor::VideoCodecUncompressedRGB)
	{
		return getVideoCodecInfo(UNCOMPRESSED);
	}
	
	throw MoxMxf::InputExc("Unknown video codec");
}


InputFile::InputFile(MoxMxf::IOStream &infile) :
	_mxf_file(infile)
{
	_header.duration() = _mxf_file.getDuration();
	_header.frameRate() = _mxf_file.getEditRate();
	
	const MoxMxf::InputFile::TrackMap &tracks = _mxf_file.getTracks();
	
	for(MoxMxf::InputFile::TrackMap::const_iterator t = tracks.begin(); t != tracks.end(); ++t)
	{
		const MoxMxf::Track *track = t->second;
		
		const MoxMxf::Track::TrackType type = track->getType();
		
		if(type == MoxMxf::Track::TrackTypePictureEssence)
		{
			const MoxMxf::PictureTrack &vid_track = dynamic_cast<const MoxMxf::PictureTrack &>(*track);
			
			_bodySID = vid_track.getBodySID();
			_indexSID = vid_track.getIndexSID();
			
			MoxMxf::VideoDescriptor *video_descriptor = dynamic_cast<MoxMxf::VideoDescriptor *>( vid_track.getDescriptor() );
			
			if(video_descriptor == NULL)
				throw MoxMxf::NullExc("Could not get descriptor.");
			
			
			_header.displayWindow() = Box2i(V2i(0, 0), V2i(video_descriptor->getWidth() - 1, video_descriptor->getHeight() - 1));
			
		
			ChannelList channels;
			
			channels.insert("R", Channel(UINT8));
			channels.insert("G", Channel(UINT8));
			channels.insert("B", Channel(UINT8));
			
			const VideoCodecInfo &codec_info = getVideoCodecInfo( video_descriptor->getVideoCodec() );
			
			VideoCodec *codec = codec_info.createCodec(video_descriptor, _header, channels);
			
			if(codec == NULL)
				throw MoxMxf::NullExc("Codec not created.");
								
			_codec_units.push_back( CodecUnit(channels, codec, vid_track.getNumber()) );
			
			_header.channels() = channels;
		}
	}
}


InputFile::~InputFile()
{
	for(std::list<CodecUnit>::iterator i = _codec_units.begin(); i != _codec_units.end(); ++i)
	{
		CodecUnit &unit = *i;
		
		delete unit.codec;
	}
}


void
InputFile::getFrame(int frameNumber, FrameBuffer &frameBuffer)
{
	MoxMxf::FramePtr mxf_frame = _mxf_file.getFrame(frameNumber, _bodySID, _indexSID);
	
	if(!mxf_frame)
		throw MoxMxf::NullExc("NULL frame");
	
	MoxMxf::Frame::FrameParts &frameParts = mxf_frame->getFrameParts();
	
	for(std::list<CodecUnit>::iterator u = _codec_units.begin(); u != _codec_units.end(); ++u)
	{
		CodecUnit &unit = *u;
		
		if(frameParts.find(unit.trackNumber) != frameParts.end())
		{
			MoxMxf::FramePartPtr part = frameParts[unit.trackNumber];
			
			if(!part)
				throw MoxMxf::NullExc("Null part?!?");
			
			mxflib::DataChunk &data = part->getData();
			
			unit.codec->decompress(data);
			
			FrameBufferPtr decompressed_frame = unit.codec->getNextFrame();
			
			if(!decompressed_frame)
				throw MoxMxf::NullExc("Not handling sparce frames");
			
			frameBuffer.copyFromFrame(*decompressed_frame);
		}
		else
			assert(false);
	}
}


} // namespace