/*
 *  MemoryFile.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 6/24/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_MEMORYFILE_H
#define MOXFILES_MEMORYFILE_H

#include <MoxFiles/Types.h>

namespace MoxFiles
{
	class MemoryFile
	{
	  public:
		MemoryFile(size_t granularity = (1024 * 1024)); // writing
		MemoryFile(const DataChunk &data); // reading
		~MemoryFile();
		
		void FileSeek(MoxMxf::UInt64 offset);
		MoxMxf::UInt64 FileRead(unsigned char *dest, MoxMxf::UInt64 size);
		unsigned char *MemoryRead(MoxMxf::UInt64 size);
		MoxMxf::UInt64 FileWrite(const unsigned char *source, MoxMxf::UInt64 size);
		MoxMxf::UInt64 FileTell();
		MoxMxf::Int64 FileSize();
		
		DataChunkPtr getDataChunk() const; // only applies to files we wrote
		
	  private:
		const DataChunk * const _read_data;
		DataChunk *_write_data;
		DataChunkPtr _storage;
		
		MoxMxf::UInt64 _pos;
		
		bool readOnly() const;
		const DataChunk &activeChunk() const;
	};

} // namespace

#endif // MOXFILES_MEMORYFILE_H
