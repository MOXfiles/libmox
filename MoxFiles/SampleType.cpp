/*
 *  SampleType.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/29/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/SampleType.h>

#include <MoxFiles/Types.h>
#include <MoxMxf/Exception.h>

namespace MoxFiles
{

size_t SampleSize(SampleType type)
{
	switch(type)
	{
		case UNSIGNED8:
			return sizeof(UInt8);
		
		case SIGNED16:
			return sizeof(Int16);
		
		case SIGNED24:
			return sizeof(Int32);
		
		case SIGNED32:
			return sizeof(Int32);
		
		case AFLOAT:
			return sizeof(float);
	}
	
	throw MoxMxf::ArgExc("Unknown sample type");
}

unsigned int
SampleBits(SampleType type)
{
	switch(type)
	{
		case UNSIGNED8:
			return 8;
		
		case SIGNED16:
			return 16;
		
		case SIGNED24:
			return 24;
		
		case SIGNED32:
			return 32;
		
		case AFLOAT:
			return 32;
	}
	
	throw MoxMxf::ArgExc("Unknown sample type");
}

} // namespace


