/*
 *  PixelType.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/27/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */


#include <MoxFiles/PixelType.h>

#include <MoxMxf/Exception.h>

namespace MoxFiles
{

size_t
PixelSize(PixelType type)
{
	switch(type)
	{
		case UINT8:
			return 1;
		
		case UINT10:
		case UINT12:
		case UINT16:
		case UINT16A:
			return 2;
		
		case UINT32:
			return 4;
		
		case HALF:
			return 2;
		
		case FLOAT:
			return 4;
	}
	
	throw MoxMxf::ArgExc("Unknown pixel type");
}


unsigned int
PixelBits(PixelType type)
{
	switch(type)
	{
		case UINT8:
			return 8;
		
		case UINT10:
			return 10;
			
		case UINT12:
			return 12;
			
		case UINT16:
			return 16;
			
		case UINT16A:
			return 16;
		
		case UINT32:
			return 32;
		
		case HALF:
			return 16;
		
		case FLOAT:
			return 32;
	}
	
	throw MoxMxf::ArgExc("Unknown pixel type");
}

} // namespace
