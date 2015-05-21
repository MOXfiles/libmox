/*
 *  AudioChannelList.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/29/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/AudioChannelList.h>

#include <MoxMxf/Exception.h>

using namespace std;

namespace MoxFiles
{

AudioChannel::AudioChannel(SampleType t) :
	type(t)
{

}


bool
AudioChannel::operator == (const AudioChannel &other) const
{
	return (type == other.type);
}


size_t
AudioChannelList::size() const
{
	return _map.size();
}


SampleType
AudioChannelList::type() const
{
#ifdef NDEBUG
	for(ConstIterator i = begin(); i != end(); ++i)
	{
		const AudioChannel &chan = i.channel();
		
		return chan.type;
	}
#else
	SampleType typ;

	bool first = true;

	for(ConstIterator i = begin(); i != end(); ++i)
	{
		const AudioChannel &chan = i.channel();
		
		if(first)
		{
			typ = chan.type;
			
			first = false;
		}
		else
		{
			assert(typ = chan.type);
		}
	}
	
	return typ;
#endif
	
	throw MoxMxf::LogicExc("No channels in ChannelList");
}

void	
AudioChannelList::insert (const char name[], const AudioChannel &channel)
{
    if (name[0] == 0)
		throw MoxMxf::ArgExc("Audio channel name cannot be an empty string.");

    _map[name] = channel;
}


void	
AudioChannelList::insert (const string &name, const AudioChannel &channel)
{
    insert (name.c_str(), channel);
}


void	
AudioChannelList::erase (const char name[])
{
    if (name[0] == 0)
		throw MoxMxf::ArgExc("Audio channel name cannot be an empty string.");

	AudioChannelMap::iterator itr = _map.find(name);
	
	if(itr != _map.end())
		_map.erase(itr);
}


void	
AudioChannelList::erase (const string &name)
{
    erase (name.c_str());
}


AudioChannel &
AudioChannelList::operator [] (const char name[])
{
    AudioChannelMap::iterator i = _map.find (name);

    if (i == _map.end())
		throw MoxMxf::ArgExc("Cannot find audio channel");
	//THROW (IEX_NAMESPACE::ArgExc, "Cannot find image channel \"" << name << "\".");

    return i->second;
}


const AudioChannel &
AudioChannelList::operator [] (const char name[]) const
{
    AudioChannelMap::const_iterator i = _map.find (name);

    if (i == _map.end())
		throw MoxMxf::ArgExc("Cannot find audio channel");
	//THROW (IEX_NAMESPACE::ArgExc, "Cannot find image channel \"" << name << "\".");

    return i->second;
}


std::vector<Name>
StandardAudioChannelList(unsigned int count)
{
	std::vector<Name> chans;
	
	if(count == 1)
	{
		chans.push_back("Mono");
	}
	else if(count == 2)
	{
		chans.push_back("Left");
		chans.push_back("Right");
	}
	else if(count == 6)
	{
		chans.push_back("Left");
		chans.push_back("Right");
		chans.push_back("Center");
		chans.push_back("RearLeft");
		chans.push_back("RearRight");
		chans.push_back("LFE");
	}
	else
	{
		for(int i = 1; i <= count; i++)
		{
			std::stringstream ss;
			
			ss << "Channel" << i;
			
			chans.push_back(ss.str().c_str());
		}
	}
	
	return chans;
}


AudioChannel &
AudioChannelList::operator [] (const string &name)
{
    return this->operator[] (name.c_str());
}


const AudioChannel &
AudioChannelList::operator [] (const string &name) const
{
    return this->operator[] (name.c_str());
}


AudioChannel *
AudioChannelList::findChannel (const char name[])
{
    AudioChannelMap::iterator i = _map.find (name);
    return (i == _map.end())? 0: &i->second;
}


const AudioChannel *
AudioChannelList::findChannel (const char name[]) const
{
    AudioChannelMap::const_iterator i = _map.find (name);
    return (i == _map.end())? 0: &i->second;
}


AudioChannel *
AudioChannelList::findChannel (const string &name)
{
    return findChannel (name.c_str());
}


const AudioChannel *
AudioChannelList::findChannel (const string &name) const
{
    return findChannel (name.c_str());
}


AudioChannelList::Iterator		
AudioChannelList::begin ()
{
    return _map.begin();
}


AudioChannelList::ConstIterator	
AudioChannelList::begin () const
{
    return _map.begin();
}


AudioChannelList::Iterator
AudioChannelList::end ()
{
    return _map.end();
}


AudioChannelList::ConstIterator	
AudioChannelList::end () const
{
    return _map.end();
}


AudioChannelList::Iterator
AudioChannelList::find (const char name[])
{
    return _map.find (name);
}


AudioChannelList::ConstIterator
AudioChannelList::find (const char name[]) const
{
    return _map.find (name);
}


AudioChannelList::Iterator
AudioChannelList::find (const string &name)
{
    return find (name.c_str());
}


AudioChannelList::ConstIterator
AudioChannelList::find (const string &name) const
{
    return find (name.c_str());
}


bool		
AudioChannelList::operator == (const AudioChannelList &other) const
{
    ConstIterator i = begin();
    ConstIterator j = other.begin();

    while (i != end() && j != other.end())
    {
		if (!(i.channel() == j.channel()))
			return false;

		++i;
		++j;
    }

    return i == end() && j == other.end();
}


} // namespace


