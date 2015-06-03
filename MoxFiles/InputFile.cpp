/*
 *  InputFile.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/InputFile.h>

#include <sstream>
#include <algorithm>

namespace MoxFiles
{


InputFile::InputFile(MoxMxf::IOStream &infile) :
	_mxf_file(infile),
	_bodySID(0),
	_indexSID(0)
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
			
			if(_bodySID == 0)
			{
				_bodySID = vid_track.getBodySID();
				_indexSID = vid_track.getIndexSID();
			}
			else
			{
				assert(_bodySID == vid_track.getBodySID());
				assert(_indexSID == vid_track.getIndexSID());
			}
			
			const MoxMxf::VideoDescriptor &video_descriptor = dynamic_cast<const MoxMxf::VideoDescriptor &>( vid_track.getDescriptor() );			
		
			const VideoCodecInfo &codec_info = getVideoCodecInfo( video_descriptor.getVideoCodec() );
			
			ChannelList channelList;
			
			VideoCodec *codec = codec_info.createCodec(video_descriptor, _header, channelList);
			
			if(codec == NULL)
				throw MoxMxf::NullExc("Codec not created.");
			
			if(_video_codec_units.size() == 0)
			{
				_header.dataWindow() = codec->dataWindow();
				_header.displayWindow() = codec->displayWindow();
				
				if(codec->sampledWindow() != codec->dataWindow())
				{
					_header.insert("sampledWindow", Box2iAttribute(codec->sampledWindow()));
				}
			}
			else
			{
				assert(_header.dataWindow() == codec->dataWindow());
				assert(_header.displayWindow() == codec->displayWindow());
				
				Box2iAttribute *sampledWindowAttr = _header.findTypedAttribute<Box2iAttribute>("sampledWindow");
				
				if(sampledWindowAttr)
				{
					assert(sampledWindowAttr->value() == codec->sampledWindow());
				}
			}
			
			assert(channelList.size() > 0);
			
			_video_codec_units.push_back( VideoCodecUnit(channelList, codec, vid_track.getNumber()) );
		}
		else if(type == MoxMxf::Track::TrackTypeSoundEssence)
		{
			const MoxMxf::SoundTrack &sound_track = dynamic_cast<const MoxMxf::SoundTrack &>(*track);
			
			if(_bodySID == 0)
			{
				_bodySID = sound_track.getBodySID();
				_indexSID = sound_track.getIndexSID();
			}
			else
			{
				assert(_bodySID == sound_track.getBodySID());
				assert(_indexSID == sound_track.getIndexSID());
			}
			
			const MoxMxf::AudioDescriptor &audio_descriptor = dynamic_cast<const MoxMxf::AudioDescriptor &>( sound_track.getDescriptor() );			
			
			//_header.displayWindow() = Box2i(V2i(0, 0), V2i(video_descriptor->getWidth() - 1, video_descriptor->getHeight() - 1));
			Rational &sample_rate = _header.sampleRate();
			
			if(sample_rate == Rational(0, 1))
			{
				sample_rate = audio_descriptor.getAudioSamplingRate();
			}
			else
			{
				assert(sample_rate == audio_descriptor.getAudioSamplingRate());
			}
			
			
			const AudioCodecInfo &codec_info = getAudioCodecInfo( audio_descriptor.getAudioCodec() );
			
			
			AudioChannelList channelList;
			
			AudioCodec *codec = codec_info.createCodec(audio_descriptor, _header, channelList);
			
			if(codec == NULL)
				throw MoxMxf::NullExc("Codec not created.");
			
			/*
			SampleType sampleType;
			
			switch( audio_descriptor->getBitDepth() )
			{
				case 8:		sampleType = UNSIGNED8;		break;
				case 16:	sampleType = SIGNED16;		break;
				case 24:	sampleType = SIGNED24;		break;
				case 32:	sampleType = SIGNED32;		break;
				
				default:
					throw MoxMxf::NoImplExc("Unhandled bit depth");
			}
			
			const UInt32 num_channels = channelList.size();
			
			if(num_channels == 1)
			{
				
			}
			*/
		
			assert(channelList.size() > 0);
			
			_audio_codec_units.push_back( AudioCodecUnit(channelList, codec, sound_track.getNumber()) );
		}
	}
	
	
	if(_video_codec_units.size() > 0)
	{
		assert(_video_codec_units.size() == 1); // not handling multiple video streams yet
		
		const VideoCodecUnit &unit = _video_codec_units.front();
		
		_header.channels() = unit.channelList;
	}
	
	
	if(_audio_codec_units.size() > 0)
	{
		int total_channels = 0;
		
		for(std::list<AudioCodecUnit>::const_iterator i = _audio_codec_units.begin(); i != _audio_codec_units.end(); ++i)
		{
			const AudioCodecUnit &unit = *i;
			
			total_channels += unit.channelList.size();
		}
		
		
		std::deque<Name> channelNames;
		
		if(total_channels == 1)
		{
			channelNames.push_back("Mono");
		}
		else if(total_channels == 2)
		{
			channelNames.push_back("Left");
			channelNames.push_back("Right");
		}
		else if(total_channels == 6)
		{
			channelNames.push_back("Left");
			channelNames.push_back("Right");
			channelNames.push_back("Center");
			channelNames.push_back("RearLeft");
			channelNames.push_back("RearRight");
			channelNames.push_back("LFE");
		}
		else
		{
			for(int i = 1; i <= total_channels; i++)
			{
				std::stringstream ss;
				
				ss << "Channel" << i;
				
				channelNames.push_back(ss.str().c_str());
			}
		}
		
		
		AudioChannelList audio_channels;
		
		for(std::list<AudioCodecUnit>::iterator i = _audio_codec_units.begin(); i != _audio_codec_units.end(); ++i)
		{
			AudioCodecUnit &unit = *i;
			
			for(AudioChannelList::Iterator ch = unit.channelList.begin(); ch != unit.channelList.end(); ++ch)
			{
				if(channelNames.size() > 0)
				{
					const Name codec_name = ch.name();
					const AudioChannel &chan = ch.channel();
				
					const Name external_name = channelNames.front();
					channelNames.pop_front();
					
					unit.channel_map[codec_name] = external_name;
					
					audio_channels.insert(external_name.text(), chan);
				}
				else
					assert(false);
			}
		}
		
		assert(channelNames.size() == 0);
		
		_header.audioChannels() = audio_channels;
	}
	
	
	const UInt64 dur = _header.duration();
	
	for(UInt64 f = 0; f < dur; f++)
	{
		MoxMxf::FramePtr mxf_frame = _mxf_file.getFrame(f, _bodySID, _indexSID);
		
		if(!mxf_frame)
			throw MoxMxf::NullExc("NULL frame");
		
		MoxMxf::Frame::FrameParts &frameParts = mxf_frame->getFrameParts();
		
		for(std::list<AudioCodecUnit>::iterator i = _audio_codec_units.begin(); i != _audio_codec_units.end(); ++i)
		{
			AudioCodecUnit &unit = *i;
			
			if(frameParts.find(unit.trackNumber) != frameParts.end())
			{
				MoxMxf::FramePartPtr part = frameParts[unit.trackNumber];
				
				if(!part)
					throw MoxMxf::NullExc("NULL frame part");
					
				if(f == 0)
				{
					unit.sampleIndex.push_back(0);
				}
				
				unit.sampleIndex.push_back(unit.sampleIndex[f] + unit.codec->samplesInFrame( part->getDataSize() ));
			}
			else
				assert(false);
		}
	}
}


InputFile::~InputFile()
{
	for(std::list<VideoCodecUnit>::iterator i = _video_codec_units.begin(); i != _video_codec_units.end(); ++i)
	{
		VideoCodecUnit &unit = *i;
		
		delete unit.codec;
	}
	
	for(std::list<AudioCodecUnit>::iterator i = _audio_codec_units.begin(); i != _audio_codec_units.end(); ++i)
	{
		AudioCodecUnit &unit = *i;
		
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
	
	for(std::list<VideoCodecUnit>::iterator u = _video_codec_units.begin(); u != _video_codec_units.end(); ++u)
	{
		VideoCodecUnit &unit = *u;
		
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


void
InputFile::readAudio(UInt64 samples, AudioBuffer &buffer)
{
	assert(samples <= buffer.length());

	// make a list of channels in the buffer
	AudioChannelList fill_channels;
	
	for(AudioBuffer::ConstIterator i = buffer.begin(); i != buffer.end(); ++i)
	{
		const Name &name = i.name();
		const AudioSlice &slice = i.slice();
		
		fill_channels.insert(name.text(), AudioChannel(slice.type));
	}

	// make track-specific AudioBuffers
	std::map<MoxMxf::TrackNum, AudioBufferPtr> trackBuffers;
	
	for(std::list<AudioCodecUnit>::const_iterator u = _audio_codec_units.begin(); u != _audio_codec_units.end(); ++u)
	{
		const AudioCodecUnit &unit = *u;
	
		AudioBufferPtr trackBuf = new AudioBuffer(buffer.length());
		
		assert(unit.channel_map.size() == unit.channelList.size());
		
		for(std::map<Name, Name>::const_iterator n = unit.channel_map.begin(); n != unit.channel_map.end(); ++n)
		{
			const Name &codec_name = n->first;
			const Name &buffer_name = n->second;
			
			AudioSlice *buffer_slice = buffer.findSlice(buffer_name.text());
			
			if(buffer_slice != NULL)
			{
				trackBuf->insert(codec_name.text(), *buffer_slice);
				
				// take channel out of list if it will be filled with data
				fill_channels.erase(buffer_name.text());
			}
		}
		
		if(trackBuf->size() > 0)
		{
			trackBuffers[unit.trackNumber] = trackBuf;
		}
	}
	
	
	// fill audio channels  we don't have data for
	AudioBuffer fill_buffer(buffer.length());
	
	for(AudioChannelList::ConstIterator i = fill_channels.begin(); i != fill_channels.end(); ++i)
	{
		const Name &name = i.name();
		
		AudioSlice *slice = buffer.findSlice(name.text());
		
		if(slice)
		{
			fill_buffer.insert(name.text(), *slice);
		}
		else
			assert(false);
	}
	
	fill_buffer.fillRemaining();
	
	

	Rational edit_rate = _mxf_file.getEditRate(); // not const because operator== is not const for some reason
	const int duration = _mxf_file.getDuration();
	
	assert(edit_rate == _header.frameRate());
	assert(duration == _header.duration());
	
	
	std::map<MoxMxf::Position, MoxMxf::FramePtr> frameStore;
	
	for(std::list<AudioCodecUnit>::iterator u = _audio_codec_units.begin(); u != _audio_codec_units.end(); ++u)
	{
		AudioCodecUnit &unit = *u;
		
		Rational sample_rate = _header.sampleRate();
		assert(sample_rate == unit.codec->getDescriptor()->getAudioSamplingRate());
		
		UInt64 samples_left = samples;
		
		UInt64 current_frame = 0;
		
		assert(duration == (unit.sampleIndex.size() - 1));
		
		while(current_frame < (unit.sampleIndex.size() - 1) && _sample_num >= unit.sampleIndex[current_frame + 1])
		{
			current_frame++;
		}
		
		while(samples_left > 0 && current_frame < duration)
		{
			const UInt64 start_sample = (_sample_num > unit.sampleIndex[current_frame] ? (_sample_num - unit.sampleIndex[current_frame]) : 0);
			const UInt64 samples_this_frame = unit.sampleIndex[current_frame + 1] - unit.sampleIndex[current_frame];
			const UInt64 samples_to_read = std::min(samples_this_frame - start_sample, samples_left);
			
			std::map<MoxMxf::Position, MoxMxf::FramePtr>::iterator frame_i = frameStore.find(current_frame);
			
			MoxMxf::FramePtr mxf_frame;
			
			if(frame_i == frameStore.end())
			{
				mxf_frame = _mxf_file.getFrame(current_frame, _bodySID, _indexSID);
				
				if(mxf_frame)
					frameStore[current_frame] = mxf_frame;
			}
			else
			{
				mxf_frame = frame_i->second;
			}
			
			if(!mxf_frame)
				throw MoxMxf::NullExc("NULL frame");
			
			
			MoxMxf::Frame::FrameParts &frameParts = mxf_frame->getFrameParts();
			
			if(trackBuffers.find(unit.trackNumber) != trackBuffers.end())
			{
				if(frameParts.find(unit.trackNumber) == frameParts.end())
					throw MoxMxf::LogicExc("Should be a frameParts entry for each trackBuffer");
				
				mxflib::DataChunk &data = frameParts[unit.trackNumber]->getData();
				
				unit.codec->decompress(data);
				
				AudioBufferPtr codec_buffer = unit.codec->getNextBuffer();
				
				if(codec_buffer)
				{
					if(start_sample > 0)
						codec_buffer->fastForward(start_sample);
				
					AudioBufferPtr track_buffer = trackBuffers[unit.trackNumber];
					
					track_buffer->copyFromBuffer(*codec_buffer, samples_to_read);
				}
				else
					throw MoxMxf::LogicExc("Not currently dealing with codecs that don't return audio every time.");
			}
			
			samples_left -= samples_to_read;
			current_frame++;
		}
		
		if(trackBuffers.find(unit.trackNumber) != trackBuffers.end())
		{
			AudioBufferPtr track_buffer = trackBuffers[unit.trackNumber];
			
			track_buffer->fillRemaining();
		}
	}
	
	
	_sample_num += samples;
}

} // namespace