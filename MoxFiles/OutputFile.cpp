/*
 *  OutputFile.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/OutputFile.h>


namespace MoxFiles
{

OutputFile::OutputFile(IOStream &outfile, const Header &header) :
	_header(header),
	_mxf_file(NULL),
	_audio_frame(0)
{
	const ChannelList &video_channels = header.channels();

	if(video_channels.size() > 0)
	{
		const VideoCodecInfo &videoCodecInfo = getVideoCodecInfo( header.videoCompression() );
		
		const ChannelCapabilities codecCapabilities = videoCodecInfo.getChannelCapabilites();
		
		if(codecCapabilities & Channels_Any)
		{
			VideoCodec *codec = videoCodecInfo.createCodec(header, video_channels);
			
			MoxMxf::Descriptor *descriptor = codec->getDescriptor();
			
			const MoxMxf::UInt8 itemType = descriptor->getGCItemType();
			const MoxMxf::UInt8 elementType = descriptor->getGCElementType();
			
			const MoxMxf::TrackNum trackNumber = MoxMxf::OutputFile::TrackNumber(itemType, 1, elementType, 1);
		
			_video_codec_units.push_back( VideoCodecUnit(video_channels, codec, trackNumber) );
		}
		else
		{
			ChannelList channels_to_assign = video_channels;
			
			std::list<std::string> rgba_list;
			rgba_list.push_back("R");
			rgba_list.push_back("G");
			rgba_list.push_back("B");
			
			if(codecCapabilities & Channels_RGBA)
				rgba_list.push_back("A");
			
			ChannelList rgba_layer;
			
			for(std::list<std::string>::const_iterator s = rgba_list.begin(); s != rgba_list.end(); ++s)
			{
				const std::string &chan_name = *s;
				
				Channel *chan = channels_to_assign.findChannel(chan_name);
				
				if(chan)
				{
					rgba_layer.insert(chan_name, *chan);
					
					channels_to_assign.erase(chan_name);
				}
			}
			
			if(rgba_layer.begin() != rgba_layer.end()) // i.e. not empty
			{
				const ChannelList channelList = rgba_layer;
				
				VideoCodec *codec = videoCodecInfo.createCodec(header, channelList);
				
				// we put temporary number and counts here because we don't know how many total codecs we'll have
				const MoxMxf::TrackNum tempTrackNumber = 12345;
			
				_video_codec_units.push_back( VideoCodecUnit(channelList, codec, tempTrackNumber) );
			}
			
			// handle the other layer possibilities here...someday
			
			assert(channels_to_assign.begin() == channels_to_assign.end()); // i.e. empty, only doing RGBA for now
		}
	}
	
	
	const AudioChannelList &audio_channels = header.audioChannels();
	
	if(audio_channels.size() > 0)
	{
		const AudioCodecInfo &audioCodecInfo = getAudioCodecInfo( header.audioCompression() );
		
		const AudioChannelCapabilities codecCapabilities = audioCodecInfo.getChannelCapabilites();
		
		if(codecCapabilities & AudioChannels_Any)
		{
			AudioCodec *codec = audioCodecInfo.createCodec(header, audio_channels);
			
			MoxMxf::Descriptor *descriptor = codec->getDescriptor();
			
			const MoxMxf::UInt8 itemType = descriptor->getGCItemType();
			const MoxMxf::UInt8 elementType = descriptor->getGCElementType();
			
			const MoxMxf::TrackNum trackNumber = MoxMxf::OutputFile::TrackNumber(itemType, 1, elementType, 1);
		
			_audio_codec_units.push_back( AudioCodecUnit(audio_channels, codec, trackNumber) );
		}
		else
			throw MoxMxf::NoImplExc("Not ready to handle other cases yet");
	}
	
	if( _video_codec_units.empty() && _audio_codec_units.empty() )
		throw MoxMxf::ArgExc("No codecs to write");
		
	
	MoxMxf::OutputFile::EssenceList essence_list;
	
	const MoxMxf::UInt8 video_codec_count = _video_codec_units.size();
	MoxMxf::UInt8 video_current_num = 0;
	
	for(std::list<VideoCodecUnit>::iterator i = _video_codec_units.begin(); i != _video_codec_units.end(); ++i)
	{
		VideoCodecUnit &unit = *i;
		
		MoxMxf::Descriptor *descriptor = unit.codec->getDescriptor();
		
		const MoxMxf::UInt8 itemType = descriptor->getGCItemType();
		const MoxMxf::UInt8 elementType = descriptor->getGCElementType();
		
		// the track number was temporary
		unit.trackNumber = MoxMxf::OutputFile::TrackNumber(itemType, video_codec_count, elementType, video_current_num++);
		
		essence_list[unit.trackNumber] = descriptor;
	}
	
	const MoxMxf::UInt8 audio_codec_count = _audio_codec_units.size();
	MoxMxf::UInt8 audio_current_num = 0;
	
	for(std::list<AudioCodecUnit>::iterator i = _audio_codec_units.begin(); i != _audio_codec_units.end(); ++i)
	{
		AudioCodecUnit &unit = *i;
		
		MoxMxf::Descriptor *descriptor = unit.codec->getDescriptor();
		
		const MoxMxf::UInt8 itemType = descriptor->getGCItemType();
		const MoxMxf::UInt8 elementType = descriptor->getGCElementType();
		
		// the track number was temporary
		unit.trackNumber = MoxMxf::OutputFile::TrackNumber(itemType, audio_codec_count, elementType, audio_current_num++);
		
		essence_list[unit.trackNumber] = descriptor;
	}
	
	assert(!essence_list.empty());
	
	
	_mxf_file = new MoxMxf::OutputFile(outfile, essence_list, header.frameRate(), 0);
}


OutputFile::~OutputFile()
{
	delete _mxf_file;
	
	for(std::list<VideoCodecUnit>::iterator i = _video_codec_units.begin(); i != _video_codec_units.end(); ++i)
	{
		delete i->codec;
	}
	
	for(std::list<AudioCodecUnit>::iterator i = _audio_codec_units.begin(); i != _audio_codec_units.end(); ++i)
	{
		delete i->codec;
	}
}


void
OutputFile::pushFrame(const FrameBuffer &frame)
{
	for(std::list<VideoCodecUnit>::iterator i = _video_codec_units.begin(); i != _video_codec_units.end(); ++i)
	{
		VideoCodecUnit &unit = *i;
		
		unit.codec->compress(frame);
		
		DataChunkPtr data = unit.codec->getNextData();
		
		if(!data)
			throw MoxMxf::NoImplExc("Not handling buffered data.");
		
		_mxf_file->PushEssence(unit.trackNumber, data);
	}
}


void
OutputFile::pushAudio(const AudioBuffer &audio)
{
	const Rational &frame_rate = _header.frameRate();
	const Rational &sample_rate = _header.sampleRate();
	
	const bool uniform_audio_frames = (sample_rate.Numerator * frame_rate.Denominator) % (sample_rate.Denominator * frame_rate.Numerator) == 0;
										
	UInt64 samples_this_frame = 0;
	
	if(uniform_audio_frames)
	{
		samples_this_frame = (sample_rate.Numerator * frame_rate.Denominator) / (sample_rate.Denominator * frame_rate.Numerator);
	}
	else
	{
		const double samples_per_frame = (double)(sample_rate.Numerator * frame_rate.Denominator) / (double)(sample_rate.Denominator * frame_rate.Numerator);
		
		const UInt64 samples_so_far = ((double)_audio_frame * samples_per_frame) + 0.5;
		
		const UInt64 samples_post_frame = ((double)(_audio_frame + 1) * samples_per_frame) + 0.5;
		
		samples_this_frame = samples_post_frame - samples_so_far;
	}
	
	
	AudioBuffer in_buffer = audio;
	
	assert(in_buffer.remaining() == in_buffer.length()); // already rewound
	
	while(in_buffer.remaining() > 0)
	{
		bool buffers_filled = true;
	
		for(std::list<AudioCodecUnit>::iterator i = _audio_codec_units.begin(); i != _audio_codec_units.end(); ++i)
		{
			AudioCodecUnit &unit = *i;
			
			if(!unit.audioBuffer)
			{
				unit.audioBuffer = new AudioBuffer(samples_this_frame);
				
				for(AudioChannelList::ConstIterator c = unit.channelList.begin(); c != unit.channelList.end(); ++c)
				{
					const AudioChannel &chan = c.channel();
					const Name &namen = c.name();
					const char *name = namen.text();
					
					const size_t sample_size = SampleSize(chan.type);
					const size_t data_size = samples_this_frame * sample_size;
					
					DataChunkPtr data = new DataChunk(data_size);
					
					unit.audioBuffer->insert(name, AudioSlice(chan.type, (char *)data->Data, sample_size));
					
					unit.audioBuffer->attachData(data);
				}
			}
			else
			{
				assert(unit.audioBuffer->length() == samples_this_frame);
			}
			
			
			unit.audioBuffer->readFromBuffer(in_buffer);
			
			if(unit.audioBuffer->remaining() > 0)
				buffers_filled = false;
		}
		
		assert(buffers_filled || in_buffer.remaining() == 0);
		
		if(buffers_filled)
		{
			for(std::list<AudioCodecUnit>::iterator i = _audio_codec_units.begin(); i != _audio_codec_units.end(); ++i)
			{
				AudioCodecUnit &unit = *i;
				
				unit.codec->compress(*unit.audioBuffer);
				
				DataChunkPtr data = unit.codec->getNextData();
				
				if(data)
				{
					_mxf_file->PushEssence(unit.trackNumber, data);
				}
				else
					assert(false); // expect to always get data right now
				
				unit.audioBuffer = NULL;
			}
			
			_audio_frame++;
		}
	}
}

} // namespace
