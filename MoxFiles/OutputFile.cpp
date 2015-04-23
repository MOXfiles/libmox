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
	_mxf_file(NULL)
{
	const VideoCodecInfo &videoCodecInfo = getVideoCodecInfo( header.videoCompression() );
	
	const ChannelCapabilities codecCapabilities = videoCodecInfo.getChannelCapabilites();
	
	if(codecCapabilities & Channels_Any)
	{
		const ChannelList channelList = header.channels();
		
		VideoCodec *codec = videoCodecInfo.createCodec(header, channelList);
		
		MoxMxf::Descriptor *descriptor = codec->getDescriptor();
		
		const MoxMxf::UInt8 itemType = descriptor->getGCItemType();
		const MoxMxf::UInt8 elementType = descriptor->getGCElementType();
		
		const MoxMxf::TrackNum trackNumber = MoxMxf::OutputFile::TrackNumber(itemType, 1, elementType, 1);
	
		_codec_units.push_back( CodecUnit(channelList, codec, trackNumber) );
	}
	else
	{
		ChannelList channels_to_assign = header.channels();
		
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
		
			_codec_units.push_back( CodecUnit(channelList, codec, tempTrackNumber) );
		}
		
		// handle the other layer possibilities here...someday
		
		assert(channels_to_assign.begin() == channels_to_assign.end()); // i.e. empty, only doing RGBA for now
	}
	
	if( _codec_units.empty() )
		throw MoxMxf::ArgExc("No codecs to write");
		
	
	MoxMxf::OutputFile::EssenceList essence_list;
	
	const MoxMxf::UInt8 codec_count = _codec_units.size();
	MoxMxf::UInt8 current_num = 0;
	
	for(std::list<CodecUnit>::iterator i = _codec_units.begin(); i != _codec_units.end(); ++i)
	{
		CodecUnit &unit = *i;
		
		MoxMxf::Descriptor *descriptor = unit.codec->getDescriptor();
		
		const MoxMxf::UInt8 itemType = descriptor->getGCItemType();
		const MoxMxf::UInt8 elementType = descriptor->getGCElementType();
		
		// the track number was temporary
		unit.trackNumber = MoxMxf::OutputFile::TrackNumber(itemType, codec_count, elementType, current_num++);
		
		essence_list[unit.trackNumber] = descriptor;
	}
	
	assert(!essence_list.empty());
	
	
	_mxf_file = new MoxMxf::OutputFile(outfile, essence_list, header.frameRate(), 0);
}

OutputFile::~OutputFile()
{
	delete _mxf_file;
	
	for(std::list<CodecUnit>::iterator i = _codec_units.begin(); i != _codec_units.end(); ++i)
	{
		delete i->codec;
	}
}


void
OutputFile::pushFrame(const FrameBufferPtr frame)
{
	for(std::list<CodecUnit>::iterator i = _codec_units.begin(); i != _codec_units.end(); ++i)
	{
		CodecUnit &unit = *i;
		
		DataChunkPtr data = unit.codec->compress(frame);
		
		_mxf_file->PushEssence(unit.trackNumber, data);
	}
}

} // namespace
