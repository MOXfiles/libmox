/*
 *  StdIOStream.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxMxf/StdIOStream.h>

#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>

namespace MoxMxf
{


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
		throw -1;
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


} // namespace
