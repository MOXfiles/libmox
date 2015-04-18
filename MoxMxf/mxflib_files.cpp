/*
 *  mxflib_files.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <mxflib/system.h>

#include <MoxMxf/StdIOStream.h>


#ifndef MXFLIB_NO_FILE_IO
#error "You need to define MXFLIB_NO_FILE_IO when using this with mxflib"
#endif


namespace mxflib
{

int FileSeek(FileHandle file, UInt64 offset)
{
	MoxMxf::IOStream &stream = MoxMxf::GetIOStream(file);
	
	return stream.FileSeek(offset);
}


int FileSeekEnd(FileHandle file)
{
	MoxMxf::IOStream &stream = MoxMxf::GetIOStream(file);
	
	return stream.FileSeek( stream.FileSize() );
}


UInt64 FileRead(FileHandle file, unsigned char *dest, UInt64 size)
{
	MoxMxf::IOStream &stream = MoxMxf::GetIOStream(file);
	
	return stream.FileRead(dest, size);
}


UInt64 FileWrite(FileHandle file, const unsigned char *source, UInt64 size)
{
	MoxMxf::IOStream &stream = MoxMxf::GetIOStream(file);
	
	return stream.FileWrite(source, size);
}


int FileGetc(FileHandle file)
{
	MoxMxf::IOStream &stream = MoxMxf::GetIOStream(file);
	
	UInt8 c;
	
	return (stream.FileRead(&c, 1) == 1) ? (int)c : EOF;
}


FileHandle FileOpen(const char *filename)
{
	try{
		MoxMxf::StdIOStream *stream = new MoxMxf::StdIOStream(filename, MoxMxf::StdIOStream::ReadOnly);
		
		return RegisterIOStream(stream);
	}
	catch(...) {}
	
	return 0;
}

FileHandle FileOpenRead(const char *filename)
{
	try{
		MoxMxf::StdIOStream *stream = new MoxMxf::StdIOStream(filename, MoxMxf::StdIOStream::ReadOnly);
		
		return RegisterIOStream(stream);
	}
	catch(...) {}
	
	return 0;
}


FileHandle FileOpenNew(const char *filename)
{
	try{
		MoxMxf::StdIOStream *stream = new MoxMxf::StdIOStream(filename, MoxMxf::StdIOStream::ReadWrite);
		
		return RegisterIOStream(stream);
	}
	catch(...) {}
	
	return 0;
}


bool FileValid(FileHandle file)
{
	return (file != 0);
}


bool FileEof(FileHandle file)
{
	MoxMxf::IOStream &stream = MoxMxf::GetIOStream(file);
	
	return (stream.FileTell() >= stream.FileSize());
}


UInt64 FileTell(FileHandle file)
{
	MoxMxf::IOStream &stream = MoxMxf::GetIOStream(file);
	
	return stream.FileTell();
}


void FileClose(FileHandle file)
{
	MoxMxf::DeleteIOStream(file);
}


void FileFlush(FileHandle file)
{
	MoxMxf::IOStream &stream = MoxMxf::GetIOStream(file);
	
	stream.FileFlush();
}


bool FileExists(const char *filename)
{
	struct stat buf;
	
	return stat(filename, &buf) == 0;
}


int FileDelete(const char *filename)
{
	return unlink(filename);
}


void FileTruncate(FileHandle file, Int64 newsize)
{
	MoxMxf::IOStream &stream = MoxMxf::GetIOStream(file);
	
	stream.FileTruncate(newsize);
}


Int64 FileSize(FileHandle file)
{
	MoxMxf::IOStream &stream = MoxMxf::GetIOStream(file);
	
	return stream.FileSize();
}


} // namespace
