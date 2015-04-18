/*
 *  Dictionary.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxMxf/Dictionary.h>


#include <mxflib/mxflib.h>

using namespace mxflib;

#include "mxflib/dict.h" // must put "using namespace mxflib" before this


namespace MoxMxf
{

void
InitializeDict()
{
	static bool initialized = false;
	
	if(!initialized)
	{
		LoadDictionary(DictData);
	
		initialized = true;
	}
}

} // namespace

