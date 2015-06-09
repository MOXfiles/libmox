/*
 *  StdIOStream.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxMxf/StdIOStream.h>

#include <MoxMxf/Exception.h>

#include <assert.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace MoxMxf
{

#ifdef _WIN32

StdIOStream::StdIOStream(const char *filename, Cababilities abilities) :
	_fp(-1)
{
	if(abilities == ReadWrite)
	{
		_fp = _open(filename, _O_BINARY | _O_RDWR | _O_CREAT | _O_TRUNC, _S_IREAD | _S_IWRITE);
	}
	else
	{
		_fp = _open(filename, _O_BINARY | _O_RDONLY );
	}
	
	if(_fp < 0)
		throw IoExc("Failed to open file.");
}


StdIOStream::~StdIOStream()
{
	_close(_fp);
	
	_fp = 0;
}


int
StdIOStream::FileSeek(UInt64 offset)
{
	return (_lseeki64(_fp, offset, SEEK_SET) == -1 ? -1 : 0);
}


UInt64
StdIOStream::FileRead(unsigned char *dest, UInt64 size)
{
	int Ret = _read(_fp, dest, (unsigned int)size);
	return (Ret < 0) ? static_cast<size_t>(-1) : Ret; 
}


UInt64
StdIOStream::FileWrite(const unsigned char *source, UInt64 size)
{
	int Ret = _write(_fp, source, (unsigned int)size); 
	return (Ret < 0) ? static_cast<size_t>(-1) : Ret; 
}


UInt64
StdIOStream::FileTell()
{
	return _telli64(_fp);
}


void
StdIOStream::FileFlush()
{
	_commit(_fp);
}


void
StdIOStream::FileTruncate(Int64 newsize)
{
	_chsize_s(_fp, (newsize!=-1)?((UInt64)newsize):FileTell() );
}


Int64
StdIOStream::FileSize()
{
	struct _stat64 buf;
	
	return _fstat64(_fp, &buf) != 0 ? -1 : buf.st_size;
}

#else

StdIOStream::StdIOStream(const char *filename, Cababilities abilities) :
	_fp(NULL)
{
	if(abilities == ReadWrite)
	{
		_fp = fopen(filename, "w+b");
	}
	else
	{
		_fp = fopen(filename, "rb");
	}
	
	if(_fp == NULL)
		throw IoExc("Failed to open file.");
}


StdIOStream::~StdIOStream()
{
	fclose(_fp);
	
	_fp = NULL;
}


int
StdIOStream::FileSeek(UInt64 offset)
{
	return fseeko(_fp, offset, SEEK_SET);
}


UInt64
StdIOStream::FileRead(unsigned char *dest, UInt64 size)
{
	return fread(dest, 1, size, _fp);
}


UInt64
StdIOStream::FileWrite(const unsigned char *source, UInt64 size)
{
	return fwrite(source, 1, size, _fp);
}


UInt64
StdIOStream::FileTell()
{
	return ftello(_fp);
}


void
StdIOStream::FileFlush()
{
	fflush(_fp);
}


void
StdIOStream::FileTruncate(Int64 newsize)
{
	int result = ftruncate(fileno(_fp), (newsize < 0 ? FileTell() : newsize));
	
	assert(result == 0);
}


Int64
StdIOStream::FileSize()
{
	struct stat64 buf;
	
	return fstat64(fileno(_fp), &buf) != 0 ? -1 : buf.st_size;
}

#endif // _WIN32

} // namespace
