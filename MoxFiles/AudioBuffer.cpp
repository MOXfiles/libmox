/*
 *  FrameBuffer.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/21/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/AudioBuffer.h>

#include <MoxFiles/Thread.h>

#include <MoxMxf/Exception.h>

//#include <half.h>

#include <algorithm>

using namespace std;

namespace MoxFiles
{

AudioSlice::AudioSlice (SampleType t,
              char *b,
              ptrdiff_t st)
:
    type (t),
    base (b),
    stride (st)
{
    // empty
}


AudioBuffer::AudioBuffer(UInt64 length) :
	_length(length)
{
	if(length == 0)
		throw MoxMxf::ArgExc("Invalid length for AudioBuffer");
}


AudioBuffer::AudioBuffer(const AudioBuffer &other)
{
	_map = other._map;
	_length = other._length;
	_readHeads = other._readHeads;
	_data = other._data;
}


void
AudioBuffer::fastForward(Int64 samples)
{
	for(ReadHeads::iterator i = _readHeads.begin(); i != _readHeads.end(); ++i)
	{
		if((i->second + samples > _length) || (i->second + samples < 0))
			throw MoxMxf::ArgExc("Seeking out of buffer range");
	
		i->second += samples;
	}
}


void
AudioBuffer::fastForward(const char name[], Int64 samples)
{
	ReadHeads::iterator pos = _readHeads.find(name);
	
	if(pos == _readHeads.end())
		throw MoxMxf::ArgExc("No read head for channel.");
	
	if((pos->second + samples > _length) || (pos->second + samples < 0))
		throw MoxMxf::ArgExc("Seeking out of buffer range");
	
	pos->second += samples;
}


void
AudioBuffer::fastForward(const std::string &name, Int64 samples)
{
	fastForward(name.c_str(), samples);
}



#define DECLARE_NEW_32BIT_TYPE(class_name) \
class class_name \
{ \
  public: \
	class_name() {} \
	class_name(int v) : _v(v) {} \
	operator int () const { return _v; } \
	class_name & operator = (int v) { _v = v; return *this; } \
  private: \
	int _v; \
};

DECLARE_NEW_32BIT_TYPE(Int24_t)
DECLARE_NEW_32BIT_TYPE(Int32_t)

template <typename T> struct convertinfo
{
	static int	depth();	// bit depth
	static bool	isSigned();	// signed
	static bool	isFloat();	// an unbounded format that might need clipping
	static int	max();		// largest value (absolute value)
	static int	zero();		// value to fill for zero sound
};

template<>
struct convertinfo <unsigned char>
{
	static int	depth()		{ return 8; }
	static bool	isSigned()	{ return false; }
	static bool	isFloat()	{ return false; }
	static int	max()		{ return 255; }
	static int	zero()		{ return 128; }
};

template<>
struct convertinfo <short>
{
	static int	depth()		{ return 16; }
	static bool	isSigned()	{ return true; }
	static bool	isFloat()	{ return false; }
	static int	max()		{ return 32767; }
	static int	zero()		{ return 0; }
};

template<>
struct convertinfo <Int24_t>
{
	static int	depth()		{ return 24; }
	static bool	isSigned()	{ return true; }
	static bool	isFloat()	{ return false; }
	static int	max()		{ return 8388607; }
	static int	zero()		{ return 0; }
};

template<>
struct convertinfo <Int32_t>
{
	static int	depth()		{ return 32; }
	static bool	isSigned()	{ return true; }
	static bool	isFloat()	{ return false; }
	static int	max()		{ return 2147483647; }
	static int	zero()		{ return 0; }
};

template<>
struct convertinfo <float>
{
	static int	depth()		{ return 32; }
	static bool	isSigned()	{ return true; }
	static bool	isFloat()	{ return true; }
	static int	max()		{ return 1; }
	static int	zero()		{ return 0; }
};


class AudioFillTask : public Task
{
  public:
	AudioFillTask(TaskGroup *group, const AudioSlice &slice, UInt64 length);
	virtual ~AudioFillTask() {}

	virtual void execute();

  private:
	template <typename T>
	static void Fill(char *origin, ptrdiff_t stride, UInt64 length);
  
  private:
	const AudioSlice _slice;
	const UInt64 _length;
};

AudioFillTask::AudioFillTask(TaskGroup *group, const AudioSlice &slice, UInt64 length) :
	Task(group),
	_slice(slice),
	_length(length)
{

}

void
AudioFillTask::execute()
{
	switch(_slice.type)
	{
		case UNSIGNED8:	
			Fill<unsigned char>(_slice.base, _slice.stride, _length);
		break;
		
		case SIGNED16:
			Fill<short>(_slice.base, _slice.stride, _length);
		break;

		case SIGNED24:
			Fill<Int24_t>(_slice.base, _slice.stride, _length);
		break;

		case SIGNED32:
			Fill<Int32_t>(_slice.base, _slice.stride, _length);
		break;
		
		case AFLOAT:
			Fill<float>(_slice.base, _slice.stride, _length);
		break;
	}
}

template <typename T>
void
AudioFillTask::Fill(char *origin, ptrdiff_t stride, UInt64 length)
{
	T *buf = (T *)origin;
	
	const int step = stride / sizeof(T);
	
	assert(step > 0);
	
	while(length--)
	{
		*buf = convertinfo<T>::zero();
		
		buf += step;
	}
}


class AudioCopyTask : public Task
{
  public:
	AudioCopyTask(TaskGroup *group, const AudioSlice &destination_slice, const AudioSlice &source_slice, UInt64 length);
	virtual ~AudioCopyTask() {}

	virtual void execute();

  private:
  	template <typename DSTTYPE>
	static void Copy(char *dst_origin, ptrdiff_t dst_stride, const AudioSlice &source_slice, UInt64 length);

	template <typename DSTTYPE, typename SRCTYPE>
	static void Copy(char *dst_origin, ptrdiff_t dst_stride, char *src_origin, ptrdiff_t src_stride, UInt64 length);
  
  private:
	const AudioSlice _destination_slice;
	const AudioSlice _source_slice;
	const UInt64 _length;
};

AudioCopyTask::AudioCopyTask(TaskGroup *group, const AudioSlice &destination_slice, const AudioSlice &source_slice, UInt64 length) :
	Task(group),
	_destination_slice(destination_slice),
	_source_slice(source_slice),
	_length(length)
{

}

void
AudioCopyTask::execute()
{
	switch(_destination_slice.type)
	{
		case UNSIGNED8:	
			Copy<unsigned char>(_destination_slice.base, _destination_slice.stride, _source_slice, _length);
		break;
		
		case SIGNED16:
			Copy<short>(_destination_slice.base, _destination_slice.stride, _source_slice, _length);
		break;

		case SIGNED24:
			Copy<Int24_t>(_destination_slice.base, _destination_slice.stride, _source_slice, _length);
		break;

		case SIGNED32:
			Copy<Int32_t>(_destination_slice.base, _destination_slice.stride, _source_slice, _length);
		break;
		
		case AFLOAT:
			Copy<float>(_destination_slice.base, _destination_slice.stride, _source_slice, _length);
		break;
	}
}

template<typename DSTTYPE>
void
AudioCopyTask::Copy(char *dst_origin, ptrdiff_t dst_stride, const AudioSlice &source_slice, UInt64 length)
{
	switch(source_slice.type)
	{
		case UNSIGNED8:	
			Copy<DSTTYPE, unsigned char>(dst_origin, dst_stride, source_slice.base, source_slice.stride, length);
		break;
		
		case SIGNED16:
			Copy<DSTTYPE, short>(dst_origin, dst_stride, source_slice.base, source_slice.stride, length);
		break;

		case SIGNED24:
			Copy<DSTTYPE, Int24_t>(dst_origin, dst_stride, source_slice.base, source_slice.stride, length);
		break;

		case SIGNED32:
			Copy<DSTTYPE, Int32_t>(dst_origin, dst_stride, source_slice.base, source_slice.stride, length);
		break;
		
		case AFLOAT:
			Copy<DSTTYPE, float>(dst_origin, dst_stride, source_slice.base, source_slice.stride, length);
		break;
	}
}

template <typename DSTTYPE, typename SRCTYPE>
void
AudioCopyTask::Copy(char *dst_origin, ptrdiff_t dst_stride, char *src_origin, ptrdiff_t src_stride, UInt64 length)
{
	DSTTYPE *dst = (DSTTYPE *)dst_origin;
	SRCTYPE *src = (SRCTYPE *)src_origin;
	
	const int dst_step = dst_stride / sizeof(DSTTYPE);
	const int src_step = src_stride / sizeof(SRCTYPE);
	
	if(convertinfo<DSTTYPE>::depth() == convertinfo<SRCTYPE>::depth() &&
		convertinfo<DSTTYPE>::isFloat() == convertinfo<SRCTYPE>::isFloat())
	{
		assert(convertinfo<DSTTYPE>::isSigned() == convertinfo<SRCTYPE>::isSigned());
		assert(convertinfo<DSTTYPE>::max() == convertinfo<SRCTYPE>::max());
		assert(convertinfo<DSTTYPE>::zero() == convertinfo<SRCTYPE>::zero());
	
		// same format
		while(length--)
		{
			*dst = *src;
			
			dst += dst_step;
			src += src_step;
		}
	}
	else if(convertinfo<DSTTYPE>::isFloat() == true)
	{
		assert(convertinfo<SRCTYPE>::isFloat() == false); // only have one float type right now
		
		if(convertinfo<SRCTYPE>::isSigned())
		{
			assert(convertinfo<SRCTYPE>::zero() == 0);
		
			while(length--)
			{
				*dst = (double)*src / (double)convertinfo<SRCTYPE>::max();
				
				dst += dst_step;
				src += src_step;
			}
		}
		else
		{
			assert(convertinfo<SRCTYPE>::zero() != 0);
			
			while(length--)
			{
				*dst = ((double)*src - (double)convertinfo<SRCTYPE>::zero()) / (double)(convertinfo<SRCTYPE>::max() - convertinfo<SRCTYPE>::zero());
				
				dst += dst_step;
				src += src_step;
			}
		}
	}
	else if(convertinfo<SRCTYPE>::isFloat() == true)
	{
		assert(convertinfo<DSTTYPE>::isFloat() == false); // one float type
		
		if(convertinfo<DSTTYPE>::isSigned())
		{
			while(length--)
			{
				const double clipped = max(min((double)*src, 1.0), -1.0);
			
				*dst = (clipped * (double)convertinfo<DSTTYPE>::max()) + (clipped > 0 ? 0.5 : -0.5);
				
				dst += dst_step;
				src += src_step;
			}
		}
		else
		{
			while(length--)
			{
				const double clipped = max(min((double)*src, 1.0), -1.0);
			
				*dst = (clipped * (double)(convertinfo<DSTTYPE>::max() - convertinfo<DSTTYPE>::zero())) + convertinfo<DSTTYPE>::zero() + (clipped > 0 ? 0.5 : -0.5);
				
				dst += dst_step;
				src += src_step;
			}
		}
	}
	else
	{
		assert(convertinfo<DSTTYPE>::isFloat() == false);
		assert(convertinfo<SRCTYPE>::isFloat() == false);
		
		// just convert to a float buffer and then to the destination format
		DataChunk float_Buffer(length * sizeof(float));
		
		char *float_origin = (char *)float_Buffer.Data;
		const ptrdiff_t float_stride = sizeof(float);
		
		Copy<float, SRCTYPE>(float_origin, float_stride, src_origin, src_stride, length);
		
		Copy<DSTTYPE, float>(dst_origin, dst_stride, float_origin, float_stride, length);
	}
}


void
AudioBuffer::readFromBuffer(AudioBuffer &other, UInt64 samples, bool fillMissing)
{
	TaskGroup taskGroup;
	
	for(ConstIterator i = begin(); i != end(); ++i)
	{
		const Name &namen = i.name();
		const char *name = namen.text();
		
		UInt64 copy_len = remaining(name);
		
		if(samples > 0)
			copy_len = min(copy_len, samples);
		
		assert(copy_len > 0);
					
		AudioSlice this_slice = playheadSlice(name);
		
		if(other.findSlice(name) != NULL)
		{
			AudioSlice other_slice = other.playheadSlice(name);
			
			copy_len = min(copy_len, other.remaining(name));
		
			ThreadPool::addGlobalTask(new AudioCopyTask(&taskGroup, this_slice, other_slice, copy_len));
			
			other.fastForward(name, copy_len);
		}
		else if(fillMissing)
		{
			ThreadPool::addGlobalTask(new AudioFillTask(&taskGroup, this_slice, copy_len));
		}
		
		fastForward(name, copy_len);
	}
}


void
AudioBuffer::copyFromConstBuffer(const AudioBuffer &other, UInt64 samples, bool fillMissing)
{
	TaskGroup taskGroup;
	
	for(ConstIterator i = begin(); i != end(); ++i)
	{
		const Name &namen = i.name();
		const char *name = namen.text();
		
		UInt64 copy_len = remaining(name);
		
		if(samples > 0)
			copy_len = min(copy_len, samples);
		
		assert(copy_len > 0);
					
		AudioSlice this_slice = playheadSlice(name);
		
		if(other.findSlice(name) != NULL)
		{
			AudioSlice other_slice = other.playheadSlice(name);
			
			copy_len = min(copy_len, other.remaining(name));
		
			ThreadPool::addGlobalTask(new AudioCopyTask(&taskGroup, this_slice, other_slice, copy_len));
			
			//other.fastForward(name, copy_len);
		}
		else if(fillMissing)
		{
			ThreadPool::addGlobalTask(new AudioFillTask(&taskGroup, this_slice, copy_len));
		}
		
		fastForward(name, copy_len);
	}
}


void
AudioBuffer::fillRemaining()
{
	TaskGroup taskGroup;
	
	for(ConstIterator i = begin(); i != end(); ++i)
	{
		const Name &namen = i.name();
		const char *name = namen.text();
		
		const UInt64 fill_len = remaining(name);
		
		if(fill_len > 0)
		{
			AudioSlice this_slice = playheadSlice(name);
			
			ThreadPool::addGlobalTask(new AudioFillTask(&taskGroup, this_slice, fill_len));
			
			fastForward(name, fill_len);
		}
	}
}


void
AudioBuffer::fillRemaining(const char name[])
{
	TaskGroup taskGroup;
	
	const UInt64 fill_len = remaining(name);
	
	if(fill_len > 0)
	{
		AudioSlice this_slice = playheadSlice(name);
		
		ThreadPool::addGlobalTask(new AudioFillTask(&taskGroup, this_slice, fill_len));
		
		fastForward(name, fill_len);
	}
}


void
AudioBuffer::fillRemaining(const std::string &name)
{
	fillRemaining(name.c_str());
}


void
AudioBuffer::insert (const char name[], const AudioSlice &slice)
{
    if (name[0] == 0)
    {
		throw MoxMxf::ArgExc("Frame buffer slice name cannot be an empty string.");
        //THROW (IEX_NAMESPACE::ArgExc,
        //       "Frame buffer slice name cannot be an empty string.");
    }

    _map[name] = slice;
	
	_readHeads[name] = 0;
}


void
AudioBuffer::insert (const string &name, const AudioSlice &slice)
{
    insert (name.c_str(), slice);
}


AudioSlice &
AudioBuffer::operator [] (const char name[])
{
    SliceMap::iterator i = _map.find (name);

    if (i == _map.end())
    {
		throw MoxMxf::ArgExc("Cannot find frame buffer slice.");
        //THROW (IEX_NAMESPACE::ArgExc,
        //       "Cannot find frame buffer slice \"" << name << "\".");
    }

    return i->second;
}


const AudioSlice &
AudioBuffer::operator [] (const char name[]) const
{
    SliceMap::const_iterator i = _map.find (name);

    if (i == _map.end())
    {
		throw MoxMxf::ArgExc("Cannot find frame buffer slice.");
        //THROW (IEX_NAMESPACE::ArgExc,
        //       "Cannot find frame buffer slice \"" << name << "\".");
    }

    return i->second;
}


AudioSlice &
AudioBuffer::operator [] (const string &name)
{
    return this->operator[] (name.c_str());
}


const AudioSlice &
AudioBuffer::operator [] (const string &name) const
{
    return this->operator[] (name.c_str());
}


AudioSlice *
AudioBuffer::findSlice (const char name[])
{
    SliceMap::iterator i = _map.find (name);
    return (i == _map.end())? 0: &i->second;
}


const AudioSlice *
AudioBuffer::findSlice (const char name[]) const
{
    SliceMap::const_iterator i = _map.find (name);
    return (i == _map.end())? 0: &i->second;
}


AudioSlice *
AudioBuffer::findSlice (const string &name)
{
    return findSlice (name.c_str());
}


const AudioSlice *
AudioBuffer::findSlice (const string &name) const
{
    return findSlice (name.c_str());
}


AudioSlice
AudioBuffer::playheadSlice(const char name[]) const
{
	AudioSlice slice = this->operator[] (name);
	
	ReadHeads::const_iterator pos = _readHeads.find(name);
	
	if(pos == _readHeads.end())
		throw MoxMxf::LogicExc("No read head for channel.");
		
	if(pos->second > _length)
		throw MoxMxf::LogicExc("Playhead beyond end of stream.");
	
	slice.base += (slice.stride * pos->second);
	
	return slice;
}


AudioSlice
AudioBuffer::playheadSlice(const string &name) const
{
	return playheadSlice(name.c_str());
}


UInt64
AudioBuffer::remaining() const
{
	bool first = true;

	UInt64 remain = 0;

	for(ConstIterator i = begin(); i != end(); ++i)
	{
		const Name &namen = i.name();
		const char *name = namen.text();
		
		const UInt64 this_remain = remaining(name);
		
		if(first)
		{
			remain = this_remain;
			
			first = false;
		}
		else
		{
			// whenever this is called, I *think* all buffers will have the same remaining value
			assert(remain == this_remain);
			
			remain = max(remain, this_remain);
		}
	}
	
	return remain;
}


UInt64
AudioBuffer::remaining(const char name[]) const
{
	ReadHeads::const_iterator pos = _readHeads.find(name);
	
	if(pos == _readHeads.end())
		throw MoxMxf::LogicExc("No read head for channel.");
	
	if(pos->second > _length)
		throw MoxMxf::LogicExc("Playhead beyond end of stream.");
		
	return (_length - pos->second);
}


UInt64
AudioBuffer::remaining(const string &name) const
{
	return remaining(name.c_str());
}


AudioBufferPtr
MakeAudioBufferType(const AudioBuffer &buffer, SampleType type)
{
	const UInt64 length = buffer.length();
	
	AudioBufferPtr buf = new AudioBuffer(length);
	
	for(AudioBuffer::ConstIterator i = buffer.begin(); i != buffer.end(); ++i)
	{
		const Name &name = i.name();
		const AudioSlice &slice = i.slice();
		
		const ptrdiff_t stride = SampleSize(type);
		const size_t channel_data_size = stride * length;

		DataChunkPtr chan_data = new DataChunk(channel_data_size);
		
		buf->insert(name.text(), AudioSlice(type, (char *)chan_data->Data, stride));
		
		buf->attachData(chan_data);
	}
	
	buf->copyFromBuffer(buffer);
	
	return buf;
}


AudioBuffer::Iterator
AudioBuffer::begin ()
{
    return _map.begin();
}


AudioBuffer::ConstIterator
AudioBuffer::begin () const
{
    return _map.begin();
}


AudioBuffer::Iterator
AudioBuffer::end ()
{
    return _map.end();
}


AudioBuffer::ConstIterator
AudioBuffer::end () const
{
    return _map.end();
}


AudioBuffer::Iterator
AudioBuffer::find (const char name[])
{
    return _map.find (name);
}


AudioBuffer::ConstIterator
AudioBuffer::find (const char name[]) const
{
    return _map.find (name);
}


AudioBuffer::Iterator
AudioBuffer::find (const string &name)
{
    return find (name.c_str());
}


AudioBuffer::ConstIterator
AudioBuffer::find (const string &name) const
{
    return find (name.c_str());
}


} // namespace


