/*
 *  MemoryFile.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 6/24/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/MemoryFile.h>

#include <MoxMxf/Exception.h>

namespace MoxFiles
{

MemoryFile::MemoryFile(size_t granularity) :
	_read_data(NULL),
	_write_data(NULL),
	_pos(0)
{
	_storage = _write_data = new DataChunk;
	
	_write_data->SetGranularity(granularity);
}


MemoryFile::MemoryFile(const DataChunk &data) :
	_read_data(&data),
	_write_data(NULL),
	_pos(0)
{

}


MemoryFile::~MemoryFile()
{

}


void
MemoryFile::FileSeek(MoxMxf::UInt64 offset)
{
	if( offset > FileSize() )
		throw MoxMxf::ArgExc("Seeking beyond end of file");
	
	_pos = offset;
}


MoxMxf::UInt64
MemoryFile::FileRead(unsigned char *dest, MoxMxf::UInt64 size)
{
	const size_t bytes_to_read = std::min<size_t>(size, FileSize() - _pos);
	
	const DataChunk &data = activeChunk();
	
	const unsigned char *origin = data.Data + _pos;
	
	memcpy(dest, origin, bytes_to_read);
	
	_pos += bytes_to_read;
	
	return bytes_to_read;
}


unsigned char *
MemoryFile::MemoryRead(MoxMxf::UInt64 size)
{
	const DataChunk &data = activeChunk();
	
	if(size > data.Size - _pos)
		throw MoxMxf::IoExc("MemoryRead past end of file");
	
	unsigned char *ptr = data.Data + _pos;
	
	_pos += size;
	
	return ptr;
}


MoxMxf::UInt64
MemoryFile::FileWrite(const unsigned char *source, MoxMxf::UInt64 size)
{
	if( readOnly() || _write_data == NULL)
		throw MoxMxf::ArgExc("Trying to write to read-only file");
	
	DataChunk &data = *_write_data;
	
	const size_t size_needed = _pos + size;
	
	if(data.Size < size_needed)
	{
		data.Resize(size_needed);
	}
	
	char *dest = (char *)data.Data + _pos;
	
	memcpy(dest, source, size);
	
	_pos += size;
	
	return size;
}


MoxMxf::UInt64
MemoryFile::FileTell()
{
	return _pos;
}


MoxMxf::Int64
MemoryFile::FileSize()
{
	const DataChunk &data = activeChunk();
	
	return data.Size;
}


DataChunkPtr
MemoryFile::getDataChunk() const
{
	if(!_storage)
		throw MoxMxf::NullExc("Should only call this when you've been writing.");
	
	return _storage;
}


bool
MemoryFile::readOnly() const
{
#ifndef NDEBUG
	const bool read_only = !_storage;

	if(read_only)
	{
		assert(_read_data != NULL && _write_data == NULL);
	}
	else
	{
		assert(_read_data == NULL && _write_data != NULL);
	}
	
	return read_only;
#else	
	return !_storage;
#endif
}


const DataChunk &
MemoryFile::activeChunk() const
{
	if( readOnly() )
	{
		if(_read_data == NULL || _read_data->Data == NULL)
			throw MoxMxf::LogicExc("Read Data is NULL");			
	
		return *_read_data;
	}
	else
	{
		if(_write_data == NULL)
			throw MoxMxf::LogicExc("Write Data is NULL");
			
		return *_write_data;
	}
}


} // namespace
