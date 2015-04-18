


#include <MoxMxf/IOStream.h>

#include <MoxMxf/Exception.h>

#include <map>

#include <assert.h>


namespace MoxMxf
{

typedef std::map<FileHandle, IOStream *> IOMap;

static IOMap g_map;


FileHandle
RegisterIOStream(IOStream *stream)
{
	FileHandle file = 0;
	
	while(file == 0)
	{
		FileHandle file_candidate = rand();
		
		if(g_map.find(file_candidate) == g_map.end() && file_candidate != 0)
		{
			file = file_candidate;
		
			g_map[file] = stream;
		}
	}
	
	return file;
}


IOStream &
GetIOStream(FileHandle file)
{
	if(g_map.find(file) != g_map.end())
	{
		return *g_map[file];
	}
	else
		throw LogicExc("Don't have requested FileHandle");
}


void
UnregisterIOStream(FileHandle file)
{
	IOMap::iterator stream = g_map.find(file);

	if(stream != g_map.end())
	{
		g_map.erase(stream);
	}
}


void
DeleteIOStream(FileHandle file)
{
	IOMap::iterator stream = g_map.find(file);

	if(stream != g_map.end())
	{
		delete stream->second;
	
		g_map.erase(stream);
	}
}

} // namespace


#pragma mark-


