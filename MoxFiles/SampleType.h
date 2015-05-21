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

#include <stddef.h>

namespace MoxFiles
{
	enum SampleType
	{
		UNSIGNED8  = 0,
		SIGNED16 = 1,
		SIGNED24 = 2,
		SIGNED32 = 3,
		AFLOAT   = 4,
		
		NUM_SAMPLETYPES
	};
	
	// unpacked size, so PixelSize(SIGNED24) == 4
	size_t SampleSize(SampleType type);
	
	// bit depth, SampleBits(SIGNED24) == 24
	unsigned int SampleBits(SampleType type);
}

#endif // MOXFILES_SAMPLETYPE_H
