/*
 *  FrameBuffer.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/21/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/FrameBuffer.h>

#include <MoxFiles/Thread.h>

#include <MoxMxf/Exception.h>

#include <half.h>

#include <algorithm>

using namespace std;

namespace MoxFiles
{

Slice::Slice (PixelType t,
              char *b,
              ptrdiff_t xst,
              ptrdiff_t yst,
              int xsm,
              int ysm,
              double fv,
              bool xtc,
              bool ytc)
:
    type (t),
    base (b),
    xStride (xst),
    yStride (yst),
    xSampling (xsm),
    ySampling (ysm),
    fillValue (fv),
    xTileCoords (xtc),
    yTileCoords (ytc)
{
    // empty
}


FrameBuffer::FrameBuffer(const Box2i &dataWindow) :
	_dataWindow(dataWindow)
{
	if( _dataWindow.isEmpty() )
		throw MoxMxf::ArgExc("Invalid dimensions for FrameBuffer");
}


FrameBuffer::FrameBuffer(int width, int height) :
	_dataWindow(V2i(0, 0), V2i(width - 1, height - 1))
{
	if(width < 1 || height < 1)
		throw MoxMxf::ArgExc("Invalid dimensions for FrameBuffer");
}


class FillTask : public Task
{
  public:
	FillTask(TaskGroup *group, const Slice &slice, const Box2i &dw, int y);
	virtual ~FillTask() {}

	virtual void execute();

  private:
	template <typename T>
	static void FillRow(char *origin, const T value, ptrdiff_t xStride, const int width);
  
  private:
	const Slice &_slice;
	const Box2i &_dw;
	const int _y;
};

FillTask::FillTask(TaskGroup *group, const Slice &slice, const Box2i &dw, int y) :
	Task(group),
	_slice(slice),
	_dw(dw),
	_y(y)
{

}

void
FillTask::execute()
{
	char *origin = _slice.base + (_y * _slice.yStride) + (_dw.min.x * _slice.xStride);
	
	const int width = _dw.max.x - _dw.min.x + 1;
		
	switch(_slice.type)
	{
		case UINT8:	
			FillRow<unsigned char>(origin, _slice.fillValue, _slice.xStride, width);
		break;
		
		case UINT10:
		case UINT12:
		case UINT16:
		case UINT16A:
			FillRow<unsigned short>(origin, _slice.fillValue, _slice.xStride, width);
		break;
		
		case UINT32:
			FillRow<unsigned int>(origin, _slice.fillValue, _slice.xStride, width);
		break;
		
		case HALF:
			FillRow<half>(origin, _slice.fillValue, _slice.xStride, width);
		break;

		case FLOAT:
			FillRow<float>(origin, _slice.fillValue, _slice.xStride, width);
		break;
	}
}

template <typename T>
void
FillTask::FillRow(char *origin, const T value, ptrdiff_t xStride, const int width)
{
	T *pix = (T *)origin;
	
	const int step = xStride / sizeof(T);
	
	for(int x = 0; x < width; x++)
	{
		*pix = value;
		
		pix += step;
	}
}

static void
fillSlice(TaskGroup &taskGroup,
		const Slice &slice,
		const Box2i &dw)
{
	assert(slice.xSampling == 1 && slice.ySampling == 1); // not handling this yet

	for(int y = dw.min.y; y <= dw.max.y; y++)
	{
		ThreadPool::addGlobalTask(new FillTask(&taskGroup, slice, dw, y));
	}
}


//typedef unsigned short UInt10_t;
//typedef unsigned short UInt12_t;
//typedef unsigned short UInt16_t;
//typedef unsigned short UInt16A_t;

#define DECLARE_NEW_16BIT_TYPE(class_name) \
class class_name \
{ \
  public: \
	class_name() {} \
	class_name(unsigned short v) : _v(v) {} \
	operator unsigned short () const { return _v; } \
	class_name & operator = (unsigned short v) { _v = v; return *this; } \
  private: \
	unsigned short _v; \
};

DECLARE_NEW_16BIT_TYPE(UInt10_t)
DECLARE_NEW_16BIT_TYPE(UInt12_t)
DECLARE_NEW_16BIT_TYPE(UInt16_t)
DECLARE_NEW_16BIT_TYPE(UInt16A_t)

template <typename T> struct convertinfo
{
	static int	depth();	// bit depth
	static bool	intPix();	// an integer pixel format, not float, not id channel - gets simple conversion if false
	static bool	isFloat();	// an unbounded format that might need clipping
	static int	max();		// white value for an integer pixel format
	static bool	adobe();	// Adobe 16-bit
};

template<>
struct convertinfo <unsigned char>
{
	static int	depth()		{ return 8; }
	static bool	intPix()	{ return true; }
	static bool	isFloat()	{ return false; }
	static int	max()		{ return 255; }
	static bool	adobe()		{ return false; }
};

template<>
struct convertinfo <UInt10_t>
{
	static int	depth()		{ return 10; }
	static bool	intPix()	{ return true; }
	static bool	isFloat()	{ return false; }
	static int	max()		{ return 1023; }
	static bool	adobe()		{ return false; }
};

template<>
struct convertinfo <UInt12_t>
{
	static int	depth()		{ return 12; }
	static bool	intPix()	{ return true; }
	static bool	isFloat()	{ return false; }
	static int	max()		{ return 4097; }
	static bool	adobe()		{ return false; }
};

template<>
struct convertinfo <UInt16_t>
{
	static int	depth()		{ return 16; }
	static bool	intPix()	{ return true; }
	static bool	isFloat()	{ return false; }
	static int	max()		{ return 65535; }
	static bool	adobe()		{ return false; }
};

template<>
struct convertinfo <UInt16A_t>
{
	static int	depth()		{ return 16; }
	static bool	intPix()	{ return true; }
	static bool	isFloat()	{ return false; }
	static int	max()		{ return 32768; }
	static bool	adobe()		{ return true; }
};

template<>
struct convertinfo <unsigned int>
{
	static int	depth()		{ return 32; }
	static bool	intPix()	{ return false; }
	static bool	isFloat()	{ return false; }
	static int	max()		{ return INT_MAX; }
	static bool	adobe()		{ return false; }
};

template<>
struct convertinfo <half>
{
	static int	depth()		{ return 16; }
	static bool	intPix()	{ return false; }
	static bool	isFloat()	{ return true; }
	static int	max()		{ return 1; }
	static bool	adobe()		{ return false; }
};

template<>
struct convertinfo <float>
{
	static int	depth()		{ return 32; }
	static bool	intPix()	{ return false; }
	static bool	isFloat()	{ return true; }
	static int	max()		{ return 1; }
	static bool	adobe()		{ return false; }
};


class CopyTask : public Task
{
  public:
	CopyTask(TaskGroup *group, const Slice &destination_slice, const Slice &source_slice, const Box2i &dw, int y);
	virtual ~CopyTask() {}

	virtual void execute();

  private:
  
	template <typename DSTTYPE>
	void CopyRow(char *dest_origin, ptrdiff_t dest_xStride);
	
	template <typename DSTTYPE, typename SRCTYPE>
	void CopyRow(char *dest_origin, ptrdiff_t dest_xStride, const char *source_origin, ptrdiff_t source_xStride, const int width);

  private:
	const Slice &_destination_slice;
	const Slice &_source_slice;
	const Box2i &_dw;
	const int _y;
};


CopyTask::CopyTask(TaskGroup *group, const Slice &destination_slice, const Slice &source_slice, const Box2i &dw, int y) :
	Task(group),
	_destination_slice(destination_slice),
	_source_slice(source_slice),
	_dw(dw),
	_y(y)
{

}

void
CopyTask::execute()
{
	char *dest_origin = _destination_slice.base + (_y * _destination_slice.yStride) + (_dw.min.x * _destination_slice.xStride);
	
	switch(_destination_slice.type)
	{
		case UINT8:	
			CopyRow<unsigned char>(dest_origin, _destination_slice.xStride);
		break;
		
		case UINT10:
			CopyRow<UInt10_t>(dest_origin, _destination_slice.xStride);
		break;
		
		case UINT12:
			CopyRow<UInt16_t>(dest_origin, _destination_slice.xStride);
		break;
		
		case UINT16:
			CopyRow<UInt16_t>(dest_origin, _destination_slice.xStride);
		break;
		
		case UINT16A:
			CopyRow<UInt16A_t>(dest_origin, _destination_slice.xStride);
		break;
		
		case UINT32:
			CopyRow<unsigned int>(dest_origin, _destination_slice.xStride);
		break;
		
		case HALF:
			CopyRow<half>(dest_origin, _destination_slice.xStride);
		break;

		case FLOAT:
			CopyRow<float>(dest_origin, _destination_slice.xStride);
		break;
	}
}


template <typename DSTTYPE>
void
CopyTask::CopyRow(char *dest_origin, ptrdiff_t dest_xStride)
{
	const char *source_origin = _source_slice.base + (_y * _source_slice.yStride) + (_dw.min.x * _source_slice.xStride);
	
	const int width = _dw.max.x - _dw.min.x + 1;
	
	switch(_source_slice.type)
	{
		case UINT8:	
			CopyRow<DSTTYPE, unsigned char>(dest_origin, dest_xStride, source_origin, _source_slice.xStride, width);
		break;
		
		case UINT10:
			CopyRow<DSTTYPE, UInt10_t>(dest_origin, dest_xStride, source_origin, _source_slice.xStride, width);
		break;
		
		case UINT12:
			CopyRow<DSTTYPE, UInt16_t>(dest_origin, dest_xStride, source_origin, _source_slice.xStride, width);
		break;
		
		case UINT16:
			CopyRow<DSTTYPE, UInt16_t>(dest_origin, dest_xStride, source_origin, _source_slice.xStride, width);
		break;
		
		case UINT16A:
			CopyRow<DSTTYPE, UInt16A_t>(dest_origin, dest_xStride, source_origin, _source_slice.xStride, width);
		break;
		
		case UINT32:
			CopyRow<DSTTYPE, unsigned int>(dest_origin, dest_xStride, source_origin, _source_slice.xStride, width);
		break;
		
		case HALF:
			CopyRow<DSTTYPE, half>(dest_origin, dest_xStride, source_origin, _source_slice.xStride, width);
		break;

		case FLOAT:
			CopyRow<DSTTYPE, float>(dest_origin, dest_xStride, source_origin, _source_slice.xStride, width);
		break;
	}
}

// converting from Adobe 16-bit to regular 16-bit
#define PF_HALF_CHAN16			16384

static inline unsigned short
Promote(const unsigned short &val)
{
	return (val > PF_HALF_CHAN16 ? ( (val - 1) << 1 ) + 1 : val << 1);
}

// Convert regular 16-bit to Adobe 16-bit, max val 0x8000
#define PF_MAX_CHAN16			32768

static inline unsigned short
Demote(const unsigned short &val)
{
	return (val > PF_MAX_CHAN16 ? ( (val - 1) >> 1 ) + 1 : val >> 1);
}


template <typename DSTTYPE, typename SRCTYPE>
static inline DSTTYPE
IncreaseDepth(const int out_depth, const SRCTYPE &val, const int in_depth)
{
	return (((int)val << (out_depth - in_depth)) | ((int)val >> ((in_depth * 2) - out_depth)));
}


template <typename DSTTYPE, typename SRCTYPE>
static inline DSTTYPE
ReduceDepth(const int out_depth, const SRCTYPE &val, const int in_depth)
{
	return ((int)val >> (in_depth - out_depth));
}



template <typename DSTTYPE, typename SRCTYPE>
void
CopyTask::CopyRow(char *dest_origin, ptrdiff_t dest_xStride, const char *source_origin, ptrdiff_t source_xStride, const int width)
{
	DSTTYPE *out = (DSTTYPE *)dest_origin;
	const SRCTYPE *in = (SRCTYPE *)source_origin;
	
	const int out_step = dest_xStride / sizeof(DSTTYPE);
	const int in_step = source_xStride / sizeof(SRCTYPE);
	
	if( (convertinfo<DSTTYPE>::depth() == convertinfo<SRCTYPE>::depth()) &&
		(convertinfo<DSTTYPE>::adobe() == convertinfo<SRCTYPE>::adobe()) &&
		(convertinfo<DSTTYPE>::isFloat() == convertinfo<SRCTYPE>::isFloat()))
	{
		assert(convertinfo<DSTTYPE>::intPix() == convertinfo<SRCTYPE>::intPix());
		assert(convertinfo<DSTTYPE>::max() == convertinfo<SRCTYPE>::max());
	
		// same pixel type, just copy
		for(int x = 0; x < width; x++)
		{
			*out = *in;
			
			out += out_step;
			in += in_step;
		}
	}
	else if(!convertinfo<DSTTYPE>::intPix())
	{
		if(!convertinfo<SRCTYPE>::intPix())
		{
			// float to float, copy directly
			for(int x = 0; x < width; x++)
			{
				*out = *in;
				
				out += out_step;
				in += in_step;
			}
		}
		else
		{
			// integer to float
			for(int x = 0; x < width; x++)
			{
				*out = (float)*in / (float)convertinfo<SRCTYPE>::max();
				
				out += out_step;
				in += in_step;
			}
		}
	}
	else if(!convertinfo<SRCTYPE>::intPix())
	{
		// float to integer
		for(int x = 0; x < width; x++)
		{
			const SRCTYPE clipped = (convertinfo<SRCTYPE>::isFloat() ? (SRCTYPE)max<float>(0.f, min<float>(1.f, *in)) : *in);
		
			*out = (float)clipped * (float)convertinfo<DSTTYPE>::max() + 0.5f;
			
			out += out_step;
			in += in_step;
		}
	}
	else
	{
		// two integers
		if(convertinfo<DSTTYPE>::depth() > convertinfo<SRCTYPE>::depth())
		{
			assert(!convertinfo<SRCTYPE>::adobe());
		
			for(int x = 0; x < width; x++)
			{
				*out = IncreaseDepth<DSTTYPE, SRCTYPE>(convertinfo<DSTTYPE>::depth(), *in, convertinfo<SRCTYPE>::depth());
				
				if(convertinfo<DSTTYPE>::adobe())
					*out = Demote(*out);
				
				out += out_step;
				in += in_step;
			}
		}
		else if(convertinfo<DSTTYPE>::depth() < convertinfo<SRCTYPE>::depth())
		{
			assert(!convertinfo<DSTTYPE>::adobe());
		
			for(int x = 0; x < width; x++)
			{
				const SRCTYPE val = (convertinfo<SRCTYPE>::adobe() ? (SRCTYPE)Promote(*in) : (SRCTYPE)*in);
			
				*out = ReduceDepth<DSTTYPE, SRCTYPE>(convertinfo<DSTTYPE>::depth(), val, convertinfo<SRCTYPE>::depth());
				
				out += out_step;
				in += in_step;
			}
		}
		else
		{
			assert(convertinfo<DSTTYPE>::depth() == convertinfo<SRCTYPE>::depth());
			
			if(convertinfo<DSTTYPE>::adobe() == convertinfo<SRCTYPE>::adobe())
			{
				assert(false); // should have already been captured by the matching option
			
				for(int x = 0; x < width; x++)
				{
					*out = *in;
					
					out += out_step;
					in += in_step;
				}
			}
			else
			{
				for(int x = 0; x < width; x++)
				{
					*out = (convertinfo<SRCTYPE>::adobe() ? (DSTTYPE)Promote(*in) : (DSTTYPE)*in);
					
					if(convertinfo<DSTTYPE>::adobe())
						*out = Demote(*out);
						
					out += out_step;
					in += in_step;
				}
			}
		}
	}
}


static void
copySlice(TaskGroup &taskGroup,
		const Slice &destination_slice,
		const Slice &source_slice,
		const Box2i &dw)
{
	assert(destination_slice.xSampling == 1 && destination_slice.ySampling == 1); // not handling this yet
	assert(source_slice.xSampling == 1 && source_slice.ySampling == 1); 

	for(int y = dw.min.y; y <= dw.max.y; y++)
	{
		ThreadPool::addGlobalTask(new CopyTask(&taskGroup, destination_slice, source_slice, dw, y));
	}
}

void
FrameBuffer::copyFromFrame(const FrameBuffer &other, bool fillMissing)
{
	if(_dataWindow.min.x < other._dataWindow.min.x ||
		_dataWindow.min.y < other._dataWindow.min.y ||
		_dataWindow.max.x > other._dataWindow.max.x ||
		_dataWindow.max.y > other._dataWindow.max.y)
	{
		// fill before copying
		TaskGroup taskGroup;
		
		for(ConstIterator i = begin(); i != end(); ++i)
		{
			const string &name = i.name();
			const Slice &slice = i.slice();
			
			if(fillMissing || other.findSlice(name))
			{
				fillSlice(taskGroup, slice, _dataWindow);
			}
		}
	}
	
	// we will fill the intersection
	Box2i copyBox(V2i(max(_dataWindow.min.x, other._dataWindow.min.x),
						max(_dataWindow.min.y, other._dataWindow.min.y)),
					V2i(min(_dataWindow.max.x, other._dataWindow.max.x),
						min(_dataWindow.max.y, other._dataWindow.max.y)));
	
	if( !copyBox.isEmpty() )
	{
		TaskGroup taskGroup;
		
		for(ConstIterator i = begin(); i != end(); ++i)
		{
			const string &name = i.name();
			
			const Slice &slice = i.slice();
			const Slice *other_slice = other.findSlice(name);
			
			if(other_slice)
			{
				// copy
				copySlice(taskGroup, slice, *other_slice, copyBox);
			}
			else if(fillMissing)
			{
				// fill
				fillSlice(taskGroup, slice, copyBox);
			}
		}
	}
}


void
FrameBuffer::insert (const char name[], const Slice &slice)
{
    if (name[0] == 0)
    {
		throw MoxMxf::ArgExc("Frame buffer slice name cannot be an empty string.");
        //THROW (IEX_NAMESPACE::ArgExc,
        //       "Frame buffer slice name cannot be an empty string.");
    }

    _map[name] = slice;
}


void
FrameBuffer::insert (const string &name, const Slice &slice)
{
    insert (name.c_str(), slice);
}


Slice &
FrameBuffer::operator [] (const char name[])
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


const Slice &
FrameBuffer::operator [] (const char name[]) const
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


Slice &
FrameBuffer::operator [] (const string &name)
{
    return this->operator[] (name.c_str());
}


const Slice &
FrameBuffer::operator [] (const string &name) const
{
    return this->operator[] (name.c_str());
}


Slice *
FrameBuffer::findSlice (const char name[])
{
    SliceMap::iterator i = _map.find (name);
    return (i == _map.end())? 0: &i->second;
}


const Slice *
FrameBuffer::findSlice (const char name[]) const
{
    SliceMap::const_iterator i = _map.find (name);
    return (i == _map.end())? 0: &i->second;
}


Slice *
FrameBuffer::findSlice (const string &name)
{
    return findSlice (name.c_str());
}


const Slice *
FrameBuffer::findSlice (const string &name) const
{
    return findSlice (name.c_str());
}


FrameBuffer::Iterator
FrameBuffer::begin ()
{
    return _map.begin();
}


FrameBuffer::ConstIterator
FrameBuffer::begin () const
{
    return _map.begin();
}


FrameBuffer::Iterator
FrameBuffer::end ()
{
    return _map.end();
}


FrameBuffer::ConstIterator
FrameBuffer::end () const
{
    return _map.end();
}


FrameBuffer::Iterator
FrameBuffer::find (const char name[])
{
    return _map.find (name);
}


FrameBuffer::ConstIterator
FrameBuffer::find (const char name[]) const
{
    return _map.find (name);
}


FrameBuffer::Iterator
FrameBuffer::find (const string &name)
{
    return find (name.c_str());
}


FrameBuffer::ConstIterator
FrameBuffer::find (const string &name) const
{
    return find (name.c_str());
}


} // namespace


