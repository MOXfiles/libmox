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
#include <cmath>

using std::min;
using std::max;
using std::string;

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
	_dataWindow(dataWindow),
	_coefficients(Rec709)
{
	if( _dataWindow.isEmpty() )
		throw MoxMxf::ArgExc("Invalid dimensions for FrameBuffer");
}


FrameBuffer::FrameBuffer(int width, int height) :
	_dataWindow(V2i(0, 0), V2i(width - 1, height - 1)),
	_coefficients(Rec709)
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
			CopyRow<UInt12_t>(dest_origin, _destination_slice.xStride);
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
			CopyRow<DSTTYPE, UInt12_t>(dest_origin, dest_xStride, source_origin, _source_slice.xStride, width);
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


typedef struct RGBtoYCbCr_Coefficients
{
	double Yr;
	double Yg;
	double Yb;
	double Cbr;
	double Cbg;
	double Cbb;
	double Crr;
	double Crg;
	double Crb;
	int Yadd; // 8-bit values
	int Cadd;

	RGBtoYCbCr_Coefficients(double _Yr, double _Yg, double _Yb,
							double _Cbr, double _Cbg, double _Cbb,
							double _Crr, double _Crg, double _Crb,
							int _Yadd, int _Cadd) :
								Yr(_Yr), Yg(_Yg), Yb(_Yb),
								Cbr(_Cbr), Cbg(_Cbg), Cbb(_Cbb),
								Crr(_Crr), Crg(_Crg), Crb(_Crb),
								Yadd(_Yadd), Cadd(_Cadd) {}
} RGBtoYCbCr_Coefficients;


class RGBtoYCbCrTask : public Task
{
  public:
	RGBtoYCbCrTask(TaskGroup *group,
				const Slice &destination_Y, const Slice &destination_Cb, const Slice &destination_Cr,
				const Slice &source_R, const Slice &source_G, const Slice &source_B,
				const Box2i &dw, const RGBtoYCbCr_Coefficients &coefficients, int y);
	virtual ~RGBtoYCbCrTask() {}

	virtual void execute();

  private:
  
	template <typename T>
	void CopyRow();
	
	template <typename T>
	inline T Clip(const float &val);
	
  private:
	const Slice &_destination_Y;
	const Slice &_destination_Cb;
	const Slice &_destination_Cr;
	const Slice &_source_R;
	const Slice &_source_G;
	const Slice &_source_B;
	const Box2i &_dw;
	const RGBtoYCbCr_Coefficients &_coefficients;
	const int _y;
};


RGBtoYCbCrTask::RGBtoYCbCrTask(TaskGroup *group,
				const Slice &destination_Y, const Slice &destination_Cb, const Slice &destination_Cr,
				const Slice &source_R, const Slice &source_G, const Slice &source_B,
				const Box2i &dw, const RGBtoYCbCr_Coefficients &coefficients, int y) :
	Task(group),
	_destination_Y(destination_Y),
	_destination_Cb(destination_Cb),
	_destination_Cr(destination_Cr),
	_source_R(source_R),
	_source_G(source_G),
	_source_B(source_B),
	_dw(dw),
	_coefficients(coefficients),
	_y(y)
{

}


void
RGBtoYCbCrTask::execute()
{
	switch(_destination_Y.type)
	{
		case UINT8:	
			CopyRow<unsigned char>();
		break;
		
		case UINT10:
			CopyRow<UInt10_t>();
		break;
		
		case UINT12:
			CopyRow<UInt12_t>();
		break;
		
		case UINT16:
			CopyRow<UInt16_t>();
		break;
		
		case UINT16A:
			CopyRow<UInt16A_t>();
		break;
		
		case UINT32:
			CopyRow<unsigned int>();
		break;
		
		case HALF:
			CopyRow<half>();
		break;

		case FLOAT:
			CopyRow<float>();
		break;
	}
}


template <typename T>
void
RGBtoYCbCrTask::CopyRow()
{
	char *Y_origin = _destination_Y.base + (_y * _destination_Y.yStride) + (_dw.min.x * _destination_Y.xStride);
	char *Cb_origin = _destination_Cb.base + (_y * _destination_Cb.yStride) + (_dw.min.x * _destination_Cb.xStride);
	char *Cr_origin = _destination_Cr.base + (_y * _destination_Cr.yStride) + (_dw.min.x * _destination_Cr.xStride);
	
	T *Y = (T *)Y_origin;
	T *Cb = (T *)Cb_origin;
	T *Cr = (T *)Cr_origin;
	
	const int Y_step = _destination_Y.xStride / sizeof(T);
	const int Cb_step = _destination_Cb.xStride / sizeof(T);
	const int Cr_step = _destination_Cr.xStride / sizeof(T);
	
	const char *R_origin = _source_R.base + (_y * _source_R.yStride) + (_dw.min.x * _source_R.xStride);
	const char *G_origin = _source_G.base + (_y * _source_G.yStride) + (_dw.min.x * _source_G.xStride);
	const char *B_origin = _source_B.base + (_y * _source_B.yStride) + (_dw.min.x * _source_B.xStride);
	
	const T *R = (const T *)R_origin;
	const T *G = (const T *)G_origin;
	const T *B = (const T *)B_origin;
	
	const int R_step = _source_R.xStride / sizeof(T);
	const int G_step = _source_G.xStride / sizeof(T);
	const int B_step = _source_B.xStride / sizeof(T);
	
	const RGBtoYCbCr_Coefficients &co = _coefficients;
	
	const float round = (convertinfo<T>::isFloat() ? 0.f : 0.5f);
	
	const T Yadd = ((float)co.Yadd * (float)convertinfo<T>::max() / 255.f) + round;
	const T Cadd = ((float)co.Cadd * (float)convertinfo<T>::max() / 255.f) + round;
	
	for(int x = _dw.min.x; x <= _dw.max.x; x++)
	{
		*Y = Clip<T>((float)Yadd + (co.Yr * (float)*R) + (co.Yg * (float)*G) + (co.Yb * (float)*B) + round);
		*Cb = Clip<T>((float)Cadd + (co.Cbr * (float)*R) + (co.Cbg * (float)*G) + (co.Cbb * (float)*B) + round);
		*Cr = Clip<T>((float)Cadd + (co.Crr * (float)*R) + (co.Crg * (float)*G) + (co.Crb * (float)*B) + round);
		
		Y += Y_step;
		Cb += Cb_step;
		Cr += Cr_step;
		
		R += R_step;
		G += G_step;
		B += B_step;
	}
}


template <typename T>
inline T
RGBtoYCbCrTask::Clip(const float &val)
{
	return (convertinfo<T>::isFloat() ? val : max<float>(0, min<float>(val, convertinfo<T>::max())));
}


static void
copySlices_RGBtoYCbCr(TaskGroup &taskGroup,
					const Slice &destination_Y, const Slice &destination_Cb, const Slice &destination_Cr,
					const Slice &source_R, const Slice &source_G, const Slice &source_B,
					const Box2i &dw, const RGBtoYCbCr_Coefficients &coefficients)
{
	assert(destination_Y.xSampling == 1 && destination_Y.ySampling == 1);
	assert(destination_Cb.xSampling == 1 && destination_Cb.ySampling == 1);
	assert(destination_Cr.xSampling == 1 && destination_Cr.ySampling == 1);
	assert(source_R.xSampling == 1 && source_R.ySampling == 1);
	assert(source_G.xSampling == 1 && source_G.ySampling == 1);
	assert(source_B.xSampling == 1 && source_B.ySampling == 1);
	
	assert(destination_Y.type == source_R.type);
	assert(destination_Y.type == destination_Cb.type);
	assert(destination_Y.type == destination_Cr.type);
	assert(source_R.type == source_G.type);
	assert(source_R.type == source_B.type);
	
	for(int y = dw.min.y; y <= dw.max.y; y++)
	{
		ThreadPool::addGlobalTask(new RGBtoYCbCrTask(&taskGroup,
									destination_Y, destination_Cb, destination_Cr,
									source_R, source_G, source_B,
									dw, coefficients, y));
	}
}


typedef struct YCbCrtoRGB_Coefficients
{
	double Ry;
	double Rcb;
	double Rcr;
	double Gy;
	double Gcb;
	double Gcr;
	double By;
	double Bcb;
	double Bcr;
	int Ysub; // 8-bit values
	int Csub;

	YCbCrtoRGB_Coefficients(double _Ry, double _Rcb, double _Rcr,
							double _Gy, double _Gcb, double _Gcr,
							double _By, double _Bcb, double _Bcr,
							int _Ysub, int _Csub) :
								Ry(_Ry), Rcb(_Rcb), Rcr(_Rcr),
								Gy(_Gy), Gcb(_Gcb), Gcr(_Gcr),
								By(_By), Bcb(_Bcb), Bcr(_Bcr),
								Ysub(_Ysub), Csub(_Csub) {}
} YCbCrtoRGB_Coefficients;


class YCbCrtoRGBTask : public Task
{
  public:
	YCbCrtoRGBTask(TaskGroup *group,
				const Slice &destination_R, const Slice &destination_G, const Slice &destination_B,
				const Slice &source_Y, const Slice &source_Cb, const Slice &source_Cr,
				const Box2i &dw, const YCbCrtoRGB_Coefficients &coefficients, int y);
	virtual ~YCbCrtoRGBTask() {}

	virtual void execute();

  private:
	template <typename T>
	void CopyRow();
	
	template <typename T>
	inline T Clip(const float &val);
	
  private:
	const Slice &_destination_R;
	const Slice &_destination_G;
	const Slice &_destination_B;
	const Slice &_source_Y;
	const Slice &_source_Cb;
	const Slice &_source_Cr;
	const Box2i &_dw;
	const YCbCrtoRGB_Coefficients &_coefficients;
	const int _y;
};


YCbCrtoRGBTask::YCbCrtoRGBTask(TaskGroup *group,
				const Slice &destination_R, const Slice &destination_G, const Slice &destination_B,
				const Slice &source_Y, const Slice &source_Cb, const Slice &source_Cr,
				const Box2i &dw, const YCbCrtoRGB_Coefficients &coefficients, int y) :
	Task(group),
	_destination_R(destination_R),
	_destination_G(destination_G),
	_destination_B(destination_B),
	_source_Y(source_Y),
	_source_Cb(source_Cb),
	_source_Cr(source_Cr),
	_dw(dw),
	_coefficients(coefficients),
	_y(y)
{

}


void
YCbCrtoRGBTask::execute()
{
	switch(_destination_R.type)
	{
		case UINT8:	
			CopyRow<unsigned char>();
		break;
		
		case UINT10:
			CopyRow<UInt10_t>();
		break;
		
		case UINT12:
			CopyRow<UInt12_t>();
		break;
		
		case UINT16:
			CopyRow<UInt16_t>();
		break;
		
		case UINT16A:
			CopyRow<UInt16A_t>();
		break;
		
		case UINT32:
			CopyRow<unsigned int>();
		break;
		
		case HALF:
			CopyRow<half>();
		break;

		case FLOAT:
			CopyRow<float>();
		break;
	}
}


template <typename T>
void
YCbCrtoRGBTask::CopyRow()
{
	char *R_origin = _destination_R.base + (_y * _destination_R.yStride) + (_dw.min.x * _destination_R.xStride);
	char *G_origin = _destination_G.base + (_y * _destination_G.yStride) + (_dw.min.x * _destination_G.xStride);
	char *B_origin = _destination_B.base + (_y * _destination_B.yStride) + (_dw.min.x * _destination_B.xStride);
	
	T *R = (T *)R_origin;
	T *G = (T *)G_origin;
	T *B = (T *)B_origin;
	
	const int R_step = _destination_R.xStride / sizeof(T);
	const int G_step = _destination_G.xStride / sizeof(T);
	const int B_step = _destination_B.xStride / sizeof(T);
	
	const char *Y_origin = _source_Y.base + (_y * _source_Y.yStride) + (_dw.min.x * _source_Y.xStride);
	const char *Cb_origin = _source_Cb.base + (_y * _source_Cb.yStride) + (_dw.min.x * _source_Cb.xStride);
	const char *Cr_origin = _source_Cr.base + (_y * _source_Cr.yStride) + (_dw.min.x * _source_Cr.xStride);
	
	const T *Y = (const T *)Y_origin;
	const T *Cb = (const T *)Cb_origin;
	const T *Cr = (const T *)Cr_origin;
	
	const int Y_step = _source_Y.xStride / sizeof(T);
	const int Cb_step = _source_Cb.xStride / sizeof(T);
	const int Cr_step = _source_Cr.xStride / sizeof(T);
	
	const YCbCrtoRGB_Coefficients &co = _coefficients;
	
	const float round = (convertinfo<T>::isFloat() ? 0.f : 0.5f);
	
	const T Ysub = ((float)co.Ysub * (float)convertinfo<T>::max() / 255.f) + round;
	const T Csub = ((float)co.Csub * (float)convertinfo<T>::max() / 255.f) + round;
	
	for(int x = _dw.min.x; x <= _dw.max.x; x++)
	{
		*R = Clip<T>( (co.Ry * ((float)*Y - (float)Ysub)) + (co.Rcb * ((float)*Cb - (float)Csub)) + (co.Rcr * ((float)*Cr - (float)Csub)) + round);
		*G = Clip<T>( (co.Gy * ((float)*Y - (float)Ysub)) + (co.Gcb * ((float)*Cb - (float)Csub)) + (co.Gcr * ((float)*Cr - (float)Csub)) + round);
		*B = Clip<T>( (co.By * ((float)*Y - (float)Ysub)) + (co.Bcb * ((float)*Cb - (float)Csub)) + (co.Bcr * ((float)*Cr - (float)Csub)) + round);
		
		R += R_step;
		G += G_step;
		B += B_step;
		
		Y += Y_step;
		Cb += Cb_step;
		Cr += Cr_step;
	}
}


template <typename T>
inline T
YCbCrtoRGBTask::Clip(const float &val)
{
	return (convertinfo<T>::isFloat() ? val : max<float>(0, min<float>(val, convertinfo<T>::max())));
}


static YCbCrtoRGB_Coefficients
InvertCoefficients(const RGBtoYCbCr_Coefficients &in)
{
	const M33d mat
		(in.Yr,  in.Yg,  in.Yb,
		 in.Cbr, in.Cbg, in.Cbb,
		 in.Crr, in.Crg, in.Crb);
	
	M33d inv = mat.transposed().inverse().transposed(); // transposed because Imath:M33d is column-major
	
	// there will be some rounding errors, which we'll fix in certain circumstances
	for(int i=0; i < 3; i++)
	{
		for(int j=0; j < 3; j++)
		{
			double &val = inv[i][j];
			
			assert(std::abs(1.5 - 1.0) > 0.0000001); // make sure I've got abs(double)
			
			if(std::abs(val - 1.0) < 0.0000001)
			{
				val = 1.0;
			}
			else if(std::abs(val - 0.0) < 0.0000001)
			{
				val = 0.0;
			}
		}
	}
	
	return YCbCrtoRGB_Coefficients(inv[0][0], inv[0][1], inv[0][2],
									inv[1][0], inv[1][1], inv[1][2],
									inv[2][0], inv[2][1], inv[2][2],
									in.Yadd, in.Cadd);
}


static void
copySlices_YCbCrtoRGB(TaskGroup &taskGroup,
					const Slice &destination_R, const Slice &destination_G, const Slice &destination_B,
					const Slice &source_Y, const Slice &source_Cb, const Slice &source_Cr,
					const Box2i &dw, const YCbCrtoRGB_Coefficients &coefficients)
{
	assert(destination_R.xSampling == 1 && destination_R.ySampling == 1);
	assert(destination_G.xSampling == 1 && destination_G.ySampling == 1);
	assert(destination_B.xSampling == 1 && destination_B.ySampling == 1);
	assert(source_Y.xSampling == 1 && source_Y.ySampling == 1);
	assert(source_Cb.xSampling == 1 && source_Cb.ySampling == 1);
	assert(source_Cr.xSampling == 1 && source_Cr.ySampling == 1);
	
	assert(destination_R.type == source_Y.type);
	assert(destination_R.type == destination_G.type);
	assert(destination_R.type == destination_B.type);
	assert(source_Y.type == source_Cb.type);
	assert(source_Y.type == source_Cr.type);
	
	for(int y = dw.min.y; y <= dw.max.y; y++)
	{
		ThreadPool::addGlobalTask(new YCbCrtoRGBTask(&taskGroup,
									destination_R, destination_G, destination_B,
									source_Y, source_Cb, source_Cr,
									dw, coefficients, y));
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
		
		if(this->isYCbCr() == other.isYCbCr())
		{
			if(this->isYCbCr() && this->_coefficients != other._coefficients)
			{
				assert(false); // not yet handling this
			}
			else
			{
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
		else
		{
			// Here are the Rec 601 and 709 standards documents:
			// http://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.601-7-201103-I!!PDF-E.pdf
			// http://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.709-6-201506-I!!PDF-E.pdf
			
			// A lot of this online stuff gives a low precision estimation or seems to be off
			// http://www.fourcc.org/fccyvrgb.php
			// http://www.equasys.de/colorconversion.html
			// http://www.martinreddy.net/gfx/faqs/colorconv.faq
			
			// So our approach will be to use the math from the standards documents with maximum precision.
			// This may mean there is a slight difference between how we are converting and how others have done it.
			
			static const RGBtoYCbCr_Coefficients rec601_full(0.299, 0.587, 0.114,
															-0.299 / 1.772, -0.587 / 1.772, (1.0 - 0.114) / 1.772,
															(1.0 - 0.299) / 1.402, -0.587 / 1.402, -0.114 / 1.402,
															0, 128);
			
			static const YCbCrtoRGB_Coefficients rec601_full_inv = InvertCoefficients(rec601_full);
			
			static const double sY = (219.0 / 255.0); // scale Y
			static const double sC = (224.0 / 255.0); // scale colors
			
			static const RGBtoYCbCr_Coefficients rec601(rec601_full.Yr * sY, rec601_full.Yg * sY, rec601_full.Yb * sY,
														rec601_full.Cbr * sC, rec601_full.Cbg * sC, rec601_full.Cbb * sC,
														rec601_full.Crr * sC, rec601_full.Crg * sC, rec601_full.Crb * sC,
														16, 128);
			
			static const YCbCrtoRGB_Coefficients rec601_inv = InvertCoefficients(rec601);
			
			static const RGBtoYCbCr_Coefficients rec709_full(0.2126, 0.7152, 0.0722,
															-0.2126 / 1.8556, -0.7152 / 1.8556, (1.0 - 0.0722) / 1.8556,
															(1.0 - 0.2126) / 1.5748, -0.7152 / 1.5748, -0.0722 / 1.5748,
															0, 128);
															
			static const YCbCrtoRGB_Coefficients rec709_full_inv = InvertCoefficients(rec709_full);
			
			static const RGBtoYCbCr_Coefficients rec709(rec709_full.Yr * sY, rec709_full.Yg * sY, rec709_full.Yb * sY,
														rec709_full.Cbr * sC, rec709_full.Cbg * sC, rec709_full.Cbb * sC,
														rec709_full.Crr * sC, rec709_full.Crg * sC, rec709_full.Crb * sC,
														16, 128);

			static const YCbCrtoRGB_Coefficients rec709_inv = InvertCoefficients(rec709);
			
			// here are the results of these calculations:
			//
			//  rec601_full
			//
			//  0.29900000,  0.58700000,  0.11400000
			// -0.16873589, -0.33126411,  0.50000000
			//  0.50000000, -0.41868759, -0.08131241
			//
			//
			// 	rec601_full_inv
			//
			//  1.00000000,  0.00000000,  1.40200000
			//  1.00000000, -0.34413627, -0.71413629
			//  1.00000000,  1.77200000,  0.00000000
			//
			//
			//  rec601
			//
			//  0.25678824,  0.50412941,  0.09790588
			// -0.14822290, -0.29099279,  0.43921569
			//  0.43921569, -0.36778831, -0.07142737
			//
			//
			//  rec601_inv
			//
			//  1.16438356,  0.00000000,  1.59602679
			//  1.16438356, -0.39176229, -0.81296765
			//  1.16438356,  2.01723214,  0.00000000
			//
			//
			//  rec709_full
			//
			//  0.21260000,  0.71520000,  0.07220000
			// -0.11457211, -0.38542790,  0.50000000
			//  0.50000000, -0.45415291, -0.04584709
			//
			//
			//  rec709_full_inv
			//
			//  1.00000000,  0.00000000,  1.57480000
			//  1.00000000, -0.18732427, -0.46812427
			//  1.00000000,  1.85560000,  0.00000000
			//
			//
			//  rec709
			//
			//  0.18258588,  0.61423059,  0.06200706
			// -0.10064373, -0.33857195,  0.43921569
			//  0.43921569, -0.39894216, -0.04027352
			//
			//
			//  rec709_inv
			//
			//  1.16438356,  0.00000000,  1.79274107
			//  1.16438356, -0.21324861, -0.53290933
			//  1.16438356,  2.11240179,  0.00000000
			
			if(this->isYCbCr() && !other.isYCbCr())
			{
				const RGBtoYCbCr_Coefficients &coefficients = (_coefficients == Rec601 ? rec601 :
																_coefficients == Rec601_FullRange ? rec601_full :
																_coefficients == Rec709 ? rec709 :
																rec709_full);
				
				Slice *Y_slice = this->findSlice("Y");
				Slice *Cb_slice = this->findSlice("Cb");
				Slice *Cr_slice = this->findSlice("Cr");
				
				const Slice *R_slice = other.findSlice("R");
				const Slice *G_slice = other.findSlice("G");
				const Slice *B_slice = other.findSlice("B");
				
				if(Y_slice && Cb_slice && Cr_slice && R_slice && G_slice && B_slice)
				{
					copySlices_RGBtoYCbCr(taskGroup,
											*Y_slice, *Cb_slice, *Cr_slice,
											*R_slice, *G_slice, *B_slice,
											copyBox, coefficients);
				}
				else
					assert(false);
				
				
				for(ConstIterator i = begin(); i != end(); ++i)
				{
					const string &name = i.name();
					
					if(name != "Y" && name != "Cb" && name != "Cr")
					{
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
			else if(!this->isYCbCr() && other.isYCbCr())
			{
				const YCbCrtoRGB_Coefficients &coefficients = (other.coefficients() == Rec601 ? rec601_inv :
																other.coefficients() == Rec601_FullRange ? rec601_full_inv :
																other.coefficients() == Rec709 ? rec709_inv :
																rec709_full_inv);
																
				Slice *R_slice = this->findSlice("R");
				Slice *G_slice = this->findSlice("G");
				Slice *B_slice = this->findSlice("B");
				
				const Slice *Y_slice = other.findSlice("Y");
				const Slice *Cb_slice = other.findSlice("Cb");
				const Slice *Cr_slice = other.findSlice("Cr");
				
				if(R_slice && G_slice && B_slice && Y_slice && Cb_slice && Cr_slice)
				{
					copySlices_YCbCrtoRGB(taskGroup,
											*R_slice, *G_slice, *B_slice,
											*Y_slice, *Cb_slice, *Cr_slice,
											copyBox, coefficients);
				}
				else
					assert(false);
				
				
				for(ConstIterator i = begin(); i != end(); ++i)
				{
					const string &name = i.name();
					
					if(name != "R" && name != "G" && name != "B")
					{
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
			else
				assert(false); // huh?
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


bool
FrameBuffer::isYCbCr() const
{
	return (NULL != findSlice("Y") && NULL != findSlice("Cb") && NULL != findSlice("Cr"));
}

} // namespace


