/*
 *  Compression.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/22/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_AUDIOCOMPRESSION_H
#define MOXFILES_AUDIOCOMPRESSION_H

namespace MoxFiles
{
	enum AudioCompression
	{
		PCM = 0,	// uncompressed audio
		

		NUM_AUDIO_COMPRESSION_METHODS	// number of different compression methods
	};

} // namespace

#endif // MOXFILES_AUDIOCOMPRESSION_H