/*
 *  PlatformIOStream.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/28/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxMxf/PlatformIOStream.h>

#include <MoxMxf/Exception.h>

#include <assert.h>


using namespace MoxMxf;

#ifdef __APPLE__

PlatformIOStream::PlatformIOStream(FSIORefNum refNum) :
	_refNum(refNum),
	_I_opened(false)
{
	if(_refNum == 0)
		throw LogicExc("_refNum is 0.");
	
	FileSeek(0);
}


PlatformIOStream::PlatformIOStream(const char *filepath, Cababilities abilities) :
	_I_opened(false)
{
	OSErr result = noErr;
	
	CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)filepath, strlen(filepath) + 1, FALSE);
	if(url == NULL)
		throw IoExc ("Couldn't make CFURLRef.");
	
	FSRef fsRef;
	Boolean file_exists = CFURLGetFSRef(url, &fsRef);
	CFRelease(url);
	
	if(abilities == ReadWrite && file_exists)
	{
		result = FSDeleteObject(&fsRef);
		
		if(result != noErr)
			throw IoExc("Could not delete old file.");
		
		file_exists = false;
	}
	
	if(!file_exists)
	{
		if(abilities == ReadOnly)
			throw IoExc("File does not exist at path.");
		
		
		assert(abilities == ReadWrite);
		
		// find the last slash, splitting the directory from the file name
		int dir_name_len = 0;
		const char *file_name = NULL;
		int file_name_len = 0;
		
		const int len = strlen(filepath);
		
		for(int i = (len - 2); i >= 0 && file_name == NULL; i--)
		{
			if(filepath[i] == '/')
			{
				dir_name_len = i;
				file_name = &filepath[i + 1];
				file_name_len = (len - i) - 1;
			}
		}
		
		if(file_name == NULL)
			throw IoExc("Error parsing path.");
		
		CFURLRef dir_url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)filepath, dir_name_len, TRUE);
		
		FSRef parent_fsRef;
		const Boolean dir_success = CFURLGetFSRef(dir_url, &parent_fsRef);
		CFRelease(dir_url);
		
		if(dir_success)
		{
			UniChar u_filename[256];
			
			// poor-man's unicode copy
			UniChar *u = u_filename;
			for(int i=0; i < file_name_len; i++)
				*u++ = file_name[i];
			
			FSSpec my_fsSpec;
			result =  FSCreateFileUnicode(&parent_fsRef, file_name_len, u_filename, kFSCatInfoNone, NULL, &fsRef, &my_fsSpec);
			
			if(result != noErr)
				throw IoExc("Couldn't create new file.");
		}
		else
			throw IoExc("Couldn't make FSRef.");
	}
	
	HFSUniStr255 dataForkName;
	result = FSGetDataForkName(&dataForkName);

	const SInt8 perm = (abilities == ReadWrite ? fsRdWrPerm : fsRdPerm);

	result = FSOpenFork(&fsRef, dataForkName.length, dataForkName.unicode, perm, &_refNum);

	if(result != noErr || _refNum == 0)
		throw IoExc("Couldn't open file.");
	
	_I_opened = true;
}


PlatformIOStream::PlatformIOStream(const uint16_t *filepath, Cababilities abilities) :
	_I_opened(false)
{
	OSErr result = noErr;
	
	int len = 0;
	
	while(filepath[len] != '\0')
		len++;
	
	CFStringRef inStr = CFStringCreateWithCharacters(kCFAllocatorDefault, filepath, len);
	if(inStr == NULL)
		throw IoExc("Couldn't make CFStringRef.");
		
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLPOSIXPathStyle, 0);
	CFRelease(inStr);
	if(url == NULL)
		throw IoExc("Couldn't make CFURLRef.");
	
	FSRef fsRef;
	Boolean file_exists = CFURLGetFSRef(url, &fsRef);
	CFRelease(url);
	
	if(abilities == ReadWrite && file_exists)
	{
		result = FSDeleteObject(&fsRef);
		
		if(result != noErr)
			throw IoExc("Could not delete old file.");
		
		file_exists = false;
	}
	
	if(!file_exists)
	{
		if(abilities == ReadOnly)
			throw IoExc("File does not exist at path.");
		
		
		// find the last slash, splitting the directory from the file name
		int dir_name_len = 0;
		const uint16_t *file_name = NULL;
		int file_name_len = 0;
		
		for(int i = (len - 2); i >= 0 && file_name == NULL; i--)
		{
			if(filepath[i] == '/')
			{
				dir_name_len = i;
				file_name = &filepath[i + 1];
				file_name_len = (len - i) - 1;
			}
		}
		
		if(file_name == NULL)
			throw IoExc("Error parsing path.");

		CFStringRef dirStr = CFStringCreateWithCharacters(kCFAllocatorDefault, filepath, dir_name_len);
		if(dirStr == NULL)
			throw IoExc("Couldn't make CFStringRef.");

		CFURLRef dir_url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, dirStr, kCFURLPOSIXPathStyle, 0);
		CFRelease(dirStr);
		if(dir_url == NULL)
			throw IoExc("Couldn't make CFURLRef.");

		FSRef parent_fsRef;
		const Boolean dir_success = CFURLGetFSRef(dir_url, &parent_fsRef);
		CFRelease(dir_url);
		
		if(dir_success)
		{
			FSSpec my_fsSpec;
			result =  FSCreateFileUnicode(&parent_fsRef, file_name_len, file_name, kFSCatInfoNone, NULL, &fsRef, &my_fsSpec);
			
			if(result != noErr)
				throw IoExc("Couldn't create new file.");
		}
		else
			throw IoExc("Couldn't make FSRef.");
	}
	
	HFSUniStr255 dataForkName;
	result = FSGetDataForkName(&dataForkName);

	const SInt8 perm = (abilities == ReadWrite ? fsRdWrPerm : fsRdPerm);

	result = FSOpenFork(&fsRef, dataForkName.length, dataForkName.unicode, perm, &_refNum);

	if(result != noErr || _refNum == 0)
		throw IoExc("Couldn't open file for reading.");
	
	_I_opened = true;
}


PlatformIOStream::~PlatformIOStream()
{
	if(_I_opened)
	{
		OSErr result = FSCloseFork(_refNum);

		assert(result == noErr);
	}
}


int
PlatformIOStream::FileSeek(UInt64 offset)
{
	OSErr result = FSSetForkPosition(_refNum, fsFromStart, offset);

	return (result == noErr ? 0 : -1);
}


UInt64
PlatformIOStream::FileRead(unsigned char *dest, UInt64 size)
{
	ByteCount count = size;
	
	OSErr result = FSReadFork(_refNum, fsAtMark, 0, count, (void *)dest, &count);
	
	assert(count == size || result == eofErr); // mxflib will often read off the end of the file, producing errors
	
	return count;
}


UInt64
PlatformIOStream::FileWrite(const unsigned char *source, UInt64 size)
{
	ByteCount count = size;

	OSErr result = FSWriteFork(_refNum, fsAtMark, 0, count, (const void *)source, &count);

	assert(count == size || result == noErr);
	
	return count;
}


UInt64
PlatformIOStream::FileTell()
{
	SInt64 lpos;

	OSErr result = FSGetForkPosition(_refNum, &lpos);
	
	if(result != noErr)
		throw IoExc("Error calling FSGetForkPosition().");

	return lpos;
}


void
PlatformIOStream::FileFlush()
{
	OSErr result = FSFlushFork(_refNum);
	
	assert(result == noErr);
}


void
PlatformIOStream::FileTruncate(Int64 newsize)
{
	OSErr result = FSSetForkSize(_refNum, fsFromStart, newsize);
	
	assert(result == noErr);
}


Int64
PlatformIOStream::FileSize()
{
	SInt64 fork_size = 0;
	
	OSErr result = FSGetForkSize(_refNum, &fork_size);
	
	if(result != noErr)
		throw IoExc("Error calling FSGetForkSize().");
	
	return fork_size;
}

#endif // __APPLE__
