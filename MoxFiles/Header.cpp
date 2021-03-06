/*
 *  Header.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/20/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/Header.h>

#include <MoxFiles/Thread.h>


namespace MoxFiles
{

using namespace std;

static void
initialize(Header &header,
		const Box2i &dataWindow,
		const Box2i &displayWindow,
		const Rational &frameRate,
		const Rational &sampleRate,
		VideoCompression videoCompression,
		AudioCompression audioCompression,
		int duration,
		Int64 audioDuration)
{
	staticInitialize();

    header.insert("dataWindow", Box2iAttribute(dataWindow));
	header.insert("displayWindow", Box2iAttribute(displayWindow));
	header.insert("frameRate", RationalAttribute(frameRate));
	header.insert("sampleRate", RationalAttribute(sampleRate));
	header.insert("videoCompression", VideoCompressionAttribute(videoCompression));
	header.insert("audioCompression", AudioCompressionAttribute(audioCompression));
	header.insert("channels", ChannelListAttribute());
	header.insert("audiochannels", AudioChannelListAttribute());
	header.insert("duration", IntAttribute(duration));
	header.insert("audioDuration", Int64Attribute(audioDuration));
	
    header.insert("pixelAspectRatio", RationalAttribute(Rational(1, 1)));
}


Header::Header(const Box2i &dataWindow,
			const Box2i &displayWindow,
			const Rational &frameRate,
			const Rational &sampleRate,
			VideoCompression videoCompression,
			AudioCompression audioCompression,
			int duration,
			Int64 audioDuration)
{
	initialize(*this,
			dataWindow,
			displayWindow,
			frameRate,
			sampleRate,
			videoCompression,
			audioCompression,
			duration,
			audioDuration);
}


Header::Header(int width,
			int height,
			const Rational &frameRate,
			const Rational &sampleRate,
			VideoCompression videoCompression,
			AudioCompression audioCompression,
			int duration,
			Int64 audioDuration)
{
	const Box2i dw = Box2i(V2i(0, 0), V2i(width - 1, height - 1));

	initialize(*this,
			dw,
			dw,
			frameRate,
			sampleRate,
			videoCompression,
			audioCompression,
			duration,
			audioDuration);
}


Header::Header(const Header &other): _map()
{
    for (AttributeMap::const_iterator i = other._map.begin();
	 i != other._map.end();
	 ++i)
    {
	insert (*i->first, *i->second);
    }
}


Header::~Header()
{
    for (AttributeMap::iterator i = _map.begin();
	 i != _map.end();
	 ++i)
    {
	 delete i->second;
    }
}


Header &		
Header::operator = (const Header &other)
{
    if (this != &other)
    {
	for (AttributeMap::iterator i = _map.begin();
	     i != _map.end();
	     ++i)
	{
	     delete i->second;
	}

	_map.erase (_map.begin(), _map.end());

	for (AttributeMap::const_iterator i = other._map.begin();
	     i != other._map.end();
	     ++i)
	{
	    insert (*i->first, *i->second);
	}
    }

    return *this;
}


void
Header::erase (const char name[])
{
    if (name[0] == 0)
        throw MoxMxf::ArgExc("Image attribute name cannot be an empty string.");
    
    
    AttributeMap::iterator i = _map.find (name);
    if (i != _map.end())
        _map.erase (i);

}


void
Header::erase (const string &name)
{
    erase (name.c_str());
}
    
    
void
Header::insert (const char name[], const Attribute &attribute)
{
    if (name[0] == 0)
		throw MoxMxf::ArgExc("Image attribute name cannot be an empty string.");

    AttributeMap::iterator i = _map.find (name);

    if (i == _map.end())
    {
	Attribute *tmp = attribute.copy();

	try
	{
	    _map[name] = tmp;
	}
	catch (...)
	{
	    delete tmp;
	    throw;
	}
    }
    else
    {
	if (strcmp (i->second->typeName(), attribute.typeName()))
		throw MoxMxf::TypeExc("Cannot assign a value to attribute");
	    //THROW (IEX_NAMESPACE::TypeExc, "Cannot assign a value of "
		//		 "type \"" << attribute.typeName() << "\" "
		//		 "to image attribute \"" << name << "\" of "
		//		 "type \"" << i->second->typeName() << "\".");

	Attribute *tmp = attribute.copy();
	delete i->second;
	i->second = tmp;
    }
}


void
Header::insert (const string &name, const Attribute &attribute)
{
    insert (name.c_str(), attribute);
}


Attribute &		
Header::operator [] (const char name[])
{
    AttributeMap::iterator i = _map.find (name);

    if (i == _map.end())
		throw MoxMxf::ArgExc("Cannot find image attribute");
	//THROW (IEX_NAMESPACE::ArgExc, "Cannot find image attribute \"" << name << "\".");

    return *i->second;
}


const Attribute &	
Header::operator [] (const char name[]) const
{
    AttributeMap::const_iterator i = _map.find (name);

    if (i == _map.end())
		throw MoxMxf::ArgExc("Cannot find image attribute");
	//THROW (IEX_NAMESPACE::ArgExc, "Cannot find image attribute \"" << name << "\".");

    return *i->second;
}


Attribute &		
Header::operator [] (const string &name)
{
    return this->operator[] (name.c_str());
}


const Attribute &	
Header::operator [] (const string &name) const
{
    return this->operator[] (name.c_str());
}


Header::Iterator
Header::begin ()
{
    return _map.begin();
}


Header::ConstIterator
Header::begin () const
{
    return _map.begin();
}


Header::Iterator
Header::end ()
{
    return _map.end();
}


Header::ConstIterator
Header::end () const
{
    return _map.end();
}


Header::Iterator
Header::find (const char name[])
{
    return _map.find (name);
}


Header::ConstIterator
Header::find (const char name[]) const
{
    return _map.find (name);
}


Header::Iterator
Header::find (const string &name)
{
    return find (name.c_str());
}


Header::ConstIterator
Header::find (const string &name) const
{
    return find (name.c_str());
}


Box2i &	
Header::dataWindow ()
{
    return static_cast <Box2iAttribute &>
	((*this)["dataWindow"]).value();
}


const Box2i &
Header::dataWindow () const
{
    return static_cast <const Box2iAttribute &>
	((*this)["dataWindow"]).value();
}


Box2i &	
Header::displayWindow ()
{
    return static_cast <Box2iAttribute &>
	((*this)["displayWindow"]).value();
}


const Box2i &
Header::displayWindow () const
{
    return static_cast <const Box2iAttribute &>
	((*this)["displayWindow"]).value();
}


int
Header::width() const
{
	const Box2i &dispW = displayWindow();
	
	return dispW.max.x - dispW.min.x + 1;
}


int
Header::height() const
{
	const Box2i &dispW = displayWindow();
	
	return dispW.max.y - dispW.min.y + 1;
}


int &	
Header::duration ()
{
    return static_cast <IntAttribute &>
	((*this)["duration"]).value();
}


const int &
Header::duration () const
{
    return static_cast <const IntAttribute &>
	((*this)["duration"]).value();
}


Int64 &	
Header::audioDuration ()
{
    return static_cast <Int64Attribute &>
	((*this)["audioDuration"]).value();
}


const Int64 &
Header::audioDuration () const
{
    return static_cast <const Int64Attribute &>
	((*this)["audioDuration"]).value();
}


Rational &	
Header::pixelAspectRatio ()
{
    return static_cast <RationalAttribute &>
	((*this)["pixelAspectRatio"]).value();
}


const Rational &
Header::pixelAspectRatio () const
{
    return static_cast <const RationalAttribute &>
	((*this)["pixelAspectRatio"]).value();
}


Rational &	
Header::frameRate ()
{
    return static_cast <RationalAttribute &>
	((*this)["frameRate"]).value();
}


const Rational &
Header::frameRate () const
{
    return static_cast <const RationalAttribute &>
	((*this)["frameRate"]).value();
}


Rational &	
Header::sampleRate ()
{
    return static_cast <RationalAttribute &>
	((*this)["sampleRate"]).value();
}


const Rational &
Header::sampleRate () const
{
    return static_cast <const RationalAttribute &>
	((*this)["sampleRate"]).value();
}


ChannelList &	
Header::channels ()
{
    return static_cast <ChannelListAttribute &>
	((*this)["channels"]).value();
}


const ChannelList &	
Header::channels () const
{
    return static_cast <const ChannelListAttribute &>
	((*this)["channels"]).value();
}


AudioChannelList &	
Header::audioChannels ()
{
    return static_cast <AudioChannelListAttribute &>
	((*this)["audiochannels"]).value();
}


const AudioChannelList &	
Header::audioChannels () const
{
    return static_cast <const AudioChannelListAttribute &>
	((*this)["audiochannels"]).value();
}


VideoCompression &	
Header::videoCompression ()
{
    return static_cast <VideoCompressionAttribute &>
	((*this)["videoCompression"]).value();
}


const VideoCompression &	
Header::videoCompression () const
{
    return static_cast <const VideoCompressionAttribute &>
	((*this)["videoCompression"]).value();
}


AudioCompression &	
Header::audioCompression ()
{
    return static_cast <AudioCompressionAttribute &>
	((*this)["audioCompression"]).value();
}


const AudioCompression &	
Header::audioCompression () const
{
    return static_cast <const AudioCompressionAttribute &>
	((*this)["audioCompression"]).value();
}


void
Header::sanityCheck () const
{
    const Box2i &dispW = displayWindow();

    if (dispW.min.x > dispW.max.x ||
		dispW.min.y > dispW.max.y ||
		dispW.min.x <= -(INT_MAX / 2) ||
		dispW.min.y <= -(INT_MAX / 2) ||
		dispW.max.x >=  (INT_MAX / 2) ||
		dispW.max.y >=  (INT_MAX / 2))
    {
		throw MoxMxf::ArgExc("Invalid display window in image header.");
    }
	
	const Rational &par = pixelAspectRatio();
	
	if(par.Numerator <= 0 || par.Denominator <= 0)
	{
		throw MoxMxf::ArgExc("Invalid pixel aspect ratio.");
	}
	
	const Rational &fps = frameRate();
	
	if(fps.Numerator <= 0 || fps.Denominator <= 0)
	{
		throw MoxMxf::ArgExc("Invalid frame rate.");
	}
}


void
staticInitialize ()
{
    static Mutex criticalSection;
    Lock lock (criticalSection);

    static bool initialized = false;

    if(!initialized)
    {
		//
		// One-time initialization -- register
		// some predefined attribute types.
		//
		
		Box2iAttribute::registerAttributeType();
		RationalAttribute::registerAttributeType();
		VideoCompressionAttribute::registerAttributeType();
		AudioCompressionAttribute::registerAttributeType();
		ChannelListAttribute::registerAttributeType();

		initialized = true;
    }
}


} // namespace


