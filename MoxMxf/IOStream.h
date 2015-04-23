


#ifndef MOXMXF_IOSTREAM_H
#define MOXMXF_IOSTREAM_H

#include <MoxMxf/Types.h>


namespace MoxMxf
{
	class IOStream
	{
	  public:
		IOStream() {}
		virtual ~IOStream() {}
		
		virtual int FileSeek(UInt64 offset) = 0;
		virtual UInt64 FileRead(unsigned char *dest, UInt64 size) = 0;
		virtual UInt64 FileWrite(const unsigned char *source, UInt64 size) = 0;
		virtual UInt64 FileTell() = 0;
		virtual void FileFlush() = 0;
		virtual void FileTruncate(Int64 newsize) = 0;
		virtual Int64 FileSize() = 0;
	};
	
	FileHandle RegisterIOStream(IOStream *stream);
	IOStream & GetIOStream(FileHandle file);
	void UnregisterIOStream(FileHandle file);
	void DeleteIOStream(FileHandle file);
}

#ifndef MXFLIB_NO_FILE_IO
#warning "You probably want to define MXFLIB_NO_FILE_IO when using this with mxflib"
#endif


#endif // MOXMXF_IOSTREAM_H