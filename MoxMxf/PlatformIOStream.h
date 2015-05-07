/*
 *  PlatformIOStream.h
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/28/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXMXF_PLATFORMIOSTREAM_H
#define MOXMXF_PLATFORMIOSTREAM_H

#include <MoxMxf/IOStream.h>


#ifdef __APPLE__
#include <Carbon/Carbon.h>

#ifndef MAC_OS_X_VERSION_10_5
#define MAC_OS_X_VERSION_10_5 1050
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5
typedef SInt16 FSIORefNum;
#endif
#endif // __APPLE__


#ifdef WIN32
#include <Windows.h>
typedef unsigned short uint16_t;
typedef FILETIME DateTime;
#endif



namespace MoxMxf
{
	class PlatformIOStream : public IOStream
	{
	  public:
		typedef enum {
			ReadOnly,
			ReadWrite
		} Cababilities;
	
	#ifdef __APPLE__
		PlatformIOStream(FSIORefNum refNum);
	#endif
	#ifdef WIN32
		PlatformIOStream(HANDLE _hFile);
	#endif
		
		PlatformIOStream(const char *filepath, Cababilities abilities);
		PlatformIOStream(const uint16_t *filepath, Cababilities abilities);
		virtual ~PlatformIOStream();
		
		virtual int FileSeek(UInt64 offset);
		virtual UInt64 FileRead(unsigned char *dest, UInt64 size);
		virtual UInt64 FileWrite(const unsigned char *source, UInt64 size);
		virtual UInt64 FileTell();
		virtual void FileFlush();
		virtual void FileTruncate(Int64 newsize);
		virtual Int64 FileSize();
	
	  private:
		bool _I_opened;
		
	#ifdef __APPLE__
		FSIORefNum _refNum;
	#endif

	#ifdef WIN32
		HANDLE _hFile;
	#endif
	};

} // namespace

#endif