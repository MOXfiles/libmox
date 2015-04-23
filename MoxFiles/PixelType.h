/*
 *  PixelType.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/20/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_PIXELTYPE_H
#define MOXFILES_PIXELTYPE_H

namespace MoxFiles
{
	enum PixelType
	{
		UINT8   = 0,		// 8-bit unsigned int (0-255)
		UINT10  = 1,		// 10-bit unsigned int (0-1023)
		UINT12  = 2,		// 12-bit unsigned int (0-4097)
		UINT16  = 3,		// 16-bit unsigned int (0-65535)
		UINT16A = 4,		// 16-bit Adobe unsigned int (0-32768)
		UINT32  = 5,		// 32-bit unsigned int (for ID channels)
		HALF    = 6,		// half (16 bit floating point)
		FLOAT   = 7,		// float (32 bit floating point)

		NUM_PIXELTYPES	// number of different pixel types
	};
} // namespace

#endif // MOXFILES_PIXELTYPE_H
