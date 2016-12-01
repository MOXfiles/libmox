/*
 *  Compression.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/22/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_VIDEOCOMPRESSION_H
#define MOXFILES_VIDEOCOMPRESSION_H

namespace MoxFiles
{
	enum VideoCompression
	{
		UNCOMPRESSED  = 0,	// no compression
		JPEG,
		JPEG2000,
		JPEGLS,
		//JPEGXT,
		PNG,
		DPX,
		OPENEXR,
		DIRAC,
		MPEG,
		

		NUM_VIDEO_COMPRESSION_METHODS	// number of different compression methods
	};

} // namespace

#endif // MOXFILES_VIDEOCOMPRESSION_H