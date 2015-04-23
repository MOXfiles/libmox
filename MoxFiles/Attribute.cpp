/*
 *  Attribute.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/20/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/Attribute.h>

#include <MoxFiles/Thread.h>

#include <string.h>
#include <map>


namespace MoxFiles
{


Attribute::Attribute () {}


Attribute::~Attribute () {}


namespace {

struct NameCompare: std::binary_function <const char *, const char *, bool>
{
    bool
    operator () (const char *x, const char *y) const
    {
	return strcmp (x, y) < 0;
    }
};


typedef Attribute* (*Constructor)();
typedef std::map <const char *, Constructor, NameCompare> TypeMap;


class LockedTypeMap: public TypeMap
{
  public:

    Mutex mutex;
};


LockedTypeMap &
typeMap ()
{
    static Mutex criticalSection;
    Lock lock (criticalSection);

    static LockedTypeMap* typeMap = 0;

    if (typeMap == 0)
	typeMap = new LockedTypeMap ();

    return *typeMap;
}


} // namespace


bool		
Attribute::knownType (const char typeName[])
{
    LockedTypeMap& tMap = typeMap();
    Lock lock (tMap.mutex);

    return tMap.find (typeName) != tMap.end();
}


void	
Attribute::registerAttributeType (const char typeName[],
			          Attribute *(*newAttribute)())
{
    LockedTypeMap& tMap = typeMap();
    Lock lock (tMap.mutex);

    if (tMap.find (typeName) != tMap.end())
	throw MoxMxf::ArgExc("Cannot register image file attribute");
	//THROW (MoxMxf::ArgExc, "Cannot register image file attribute "
	//		    "type \"" << typeName << "\". "
	//		    "The type has already been registered.");

    tMap.insert (TypeMap::value_type (typeName, newAttribute));
}


void
Attribute::unRegisterAttributeType (const char typeName[])
{
    LockedTypeMap& tMap = typeMap();
    Lock lock (tMap.mutex);

    tMap.erase (typeName);
}


Attribute *
Attribute::newAttribute (const char typeName[])
{
    LockedTypeMap& tMap = typeMap();
    Lock lock (tMap.mutex);

    TypeMap::const_iterator i = tMap.find (typeName);

    if (i == tMap.end())
	throw MoxMxf::ArgExc("Cannot create image file attribute");
	//THROW (IEX_NAMESPACE::ArgExc, "Cannot create image file attribute of "
	//		    "unknown type \"" << typeName << "\".");

    return (i->second)();
}


template <>
const char *
Box2iAttribute::staticTypeName()
{
	return "box2i";
}

template <>
const char *
RationalAttribute::staticTypeName()
{
	return "rational";
}

template <>
const char *
VideoCompressionAttribute::staticTypeName()
{
	return "videoCompression";
}


template <>
const char *
AudioCompressionAttribute::staticTypeName()
{
	return "audioCompression";
}


template <>
const char *
ChannelListAttribute::staticTypeName()
{
	return "chlist";
}


} // namespace



