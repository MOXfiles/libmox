/*
 *  StdIOStream.h
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */


#ifndef MOXMXF_STDIOSTREAM_H
#define MOXMXF_STDIOSTREAM_H

#include <MoxMxf/IOStream.h>

#include <stdio.h>

namespace MoxMxf
{
	class StdIOStream : public IOStream
	{
	  public:
		typedef enum {
			ReadOnly,
			ReadWrite
		} Cababilities;
	
		StdIOStream(const char *filename, Cababilities abilities);
		virtual ~StdIOStream();
		
		virtual int FileSeek(UInt64 offset);
		virtual UInt64 FileRead(unsigned char *dest, UInt64 size);
		virtual UInt64 FileWrite(const unsigned char *source, UInt64 size);
		virtual UInt64 FileTell();
		virtual void FileFlush();
		virtual void FileTruncate(Int64 newsize);
		virtual Int64 FileSize();
	
	  private:
#ifdef _WIN32
		int _fp;
#else
		FILE *_fp;
#endif
	};
}


#endif // MOXMXF_STDIOSTREAM_H
