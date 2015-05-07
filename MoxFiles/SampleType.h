/*
 *  SampleType.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/29/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_SAMPLETYPE_H
#define MOXFILES_SAMPLETYPE_H

namespace MoxFiles
{
	enum SampleType
	{
		SIGNED8  = 0,
		SIGNED16 = 1,
		SIGNED24 = 2,
		SIGNED32 = 3,
		AFLOAT   = 4,
		
		NUM_SAMPLETYPES
	};
}

#endif // MOXFILES_SAMPLETYPE_H
