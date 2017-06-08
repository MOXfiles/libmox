/*
 *  FrameBuffer.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/21/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_FRAMEBUFFER_H
#define MOXFILES_FRAMEBUFFER_H

#include <MoxFiles/PixelType.h>

#include <MoxFiles/Types.h>

#include <string>
#include <map>

#include <stddef.h>

namespace MoxFiles
{

//-------------------------------------------------------
// Description of a single slice of the frame buffer:
//
// Note -- terminology: as part of a file, a component of
// an image (e.g. red, green, blue, depth etc.) is called
// a "channel".  As part of a frame buffer, an image
// component is called a "slice".
//-------------------------------------------------------

struct Slice
{
    //------------------------------
    // Data type; see ImfPixelType.h
    //------------------------------

    PixelType           type;


    //---------------------------------------------------------------------
    // Memory layout:  The address of pixel (x, y) is
    //
    //  base + (xp / xSampling) * xStride + (yp / ySampling) * yStride
    //
    // where xp and yp are computed as follows:
    //
    //  * If we are reading or writing a scanline-based file:
    //
    //      xp = x
    //      yp = y
    //
    //  * If we are reading a tile whose upper left coorner is at (xt, yt):
    //
    //      if xTileCoords is true then xp = x - xt, else xp = x
    //      if yTileCoords is true then yp = y - yt, else yp = y
    //
    //---------------------------------------------------------------------

    char *              base;
    ptrdiff_t           xStride;
    ptrdiff_t           yStride;


    //--------------------------------------------
    // Subsampling: pixel (x, y) is present in the
    // slice only if 
    //
    //  x % xSampling == 0 && y % ySampling == 0
    //
    //--------------------------------------------

    int                 xSampling;
    int                 ySampling;


    //----------------------------------------------------------
    // Default value, used to fill the slice when a file without
    // a channel that corresponds to this slice is read.
    //----------------------------------------------------------

    double              fillValue;
    

    //-------------------------------------------------------
    // For tiled files, the xTileCoords and yTileCoords flags
    // determine whether pixel addressing is performed using
    // absolute coordinates or coordinates relative to a
    // tile's upper left corner.  (See the comment on base,
    // xStride and yStride, above.)
    //
    // For scanline-based files these flags have no effect;
    // pixel addressing is always done using absolute
    // coordinates.
    //-------------------------------------------------------

    bool                xTileCoords;
    bool                yTileCoords;


    //------------
    // Constructor
    //------------

    Slice (PixelType type = UINT8,
           char * base = 0,
           ptrdiff_t xStride = 0,
           ptrdiff_t yStride = 0,
           int xSampling = 1,
           int ySampling = 1,
           double fillValue = 0.0,
           bool xTileCoords = false,
           bool yTileCoords = false);
};


class FrameBuffer : public RefCount<FrameBuffer>
{
  public:
	FrameBuffer(const Box2i &dataWindow);
	FrameBuffer(int width, int height);
	~FrameBuffer() {}
	
	void copyFromFrame(const FrameBuffer &other, bool fillMissing = true);
	
	void attachData(DataChunkPtr dat) { _data.push_back(dat);  }
	
	
	const Box2i & dataWindow () const { return _dataWindow; }
	int width() const { return (_dataWindow.max.x - _dataWindow.min.x + 1); }
	int height() const { return (_dataWindow.max.y - _dataWindow.min.y + 1); }
	
	size_t size() const { return _map.size(); }
	
	enum Coefficients
	{
		Rec601,
		Rec601_FullRange,
		Rec709,
		Rec709_FullRange
	};

	Coefficients & coefficients() { return _coefficients; }
	const Coefficients & coefficients() const { return _coefficients; }
	
    //------------
    // Add a slice
    //------------

    void                        insert (const char name[],
                                        const Slice &slice);

    void                        insert (const std::string &name,
                                        const Slice &slice);

    //----------------------------------------------------------------
    // Access to existing slices:
    //
    // [n]              Returns a reference to the slice with name n.
    //                  If no slice with name n exists, an IEX_NAMESPACE::ArgExc
    //                  is thrown.
    //
    // findSlice(n)     Returns a pointer to the slice with name n,
    //                  or 0 if no slice with name n exists.
    //
    //----------------------------------------------------------------

    Slice &                     operator [] (const char name[]);
    const Slice &               operator [] (const char name[]) const;

    Slice &                     operator [] (const std::string &name);
    const Slice &               operator [] (const std::string &name) const;

    Slice *                     findSlice (const char name[]);
    const Slice *               findSlice (const char name[]) const;

    Slice *                     findSlice (const std::string &name);
    const Slice *               findSlice (const std::string &name) const;


    //-----------------------------------------
    // Iterator-style access to existing slices
    //-----------------------------------------

    typedef std::map <Name, Slice> SliceMap;

    class Iterator;
    class ConstIterator;

    Iterator                    begin ();
    ConstIterator               begin () const;

    Iterator                    end ();
    ConstIterator               end () const;

    Iterator                    find (const char name[]);
    ConstIterator               find (const char name[]) const;

    Iterator                    find (const std::string &name);
    ConstIterator               find (const std::string &name) const;


  private:

    SliceMap                    _map;
	
	const Box2i					_dataWindow;
	
	std::list<DataChunkPtr>		_data;
	
	
	bool isYCbCr() const;
		
	Coefficients				_coefficients;
};

typedef SmartPtr<FrameBuffer> FrameBufferPtr;


//----------
// Iterators
//----------

class FrameBuffer::Iterator
{
  public:

    Iterator ();
    Iterator (const FrameBuffer::SliceMap::iterator &i);

    Iterator &                  operator ++ ();
    Iterator                    operator ++ (int);

    const char *                name () const;
    Slice &                     slice () const;

  private:

    friend class FrameBuffer::ConstIterator;

    FrameBuffer::SliceMap::iterator _i;
};


class FrameBuffer::ConstIterator
{
  public:

    ConstIterator ();
    ConstIterator (const FrameBuffer::SliceMap::const_iterator &i);
    ConstIterator (const FrameBuffer::Iterator &other);

    ConstIterator &             operator ++ ();
    ConstIterator               operator ++ (int);

    const char *                name () const;
    const Slice &               slice () const;

  private:

    friend bool operator == (const ConstIterator &, const ConstIterator &);
    friend bool operator != (const ConstIterator &, const ConstIterator &);

    FrameBuffer::SliceMap::const_iterator _i;
};


//-----------------
// Inline Functions
//-----------------

inline
FrameBuffer::Iterator::Iterator (): _i()
{
    // empty
}


inline
FrameBuffer::Iterator::Iterator (const FrameBuffer::SliceMap::iterator &i):
    _i (i)
{
    // empty
}


inline FrameBuffer::Iterator &
FrameBuffer::Iterator::operator ++ ()
{
    ++_i;
    return *this;
}


inline FrameBuffer::Iterator
FrameBuffer::Iterator::operator ++ (int)
{
    Iterator tmp = *this;
    ++_i;
    return tmp;
}


inline const char *
FrameBuffer::Iterator::name () const
{
    return *_i->first;
}


inline Slice &
FrameBuffer::Iterator::slice () const
{
    return _i->second;
}


inline
FrameBuffer::ConstIterator::ConstIterator (): _i()
{
    // empty
}

inline
FrameBuffer::ConstIterator::ConstIterator
    (const FrameBuffer::SliceMap::const_iterator &i): _i (i)
{
    // empty
}


inline
FrameBuffer::ConstIterator::ConstIterator (const FrameBuffer::Iterator &other):
    _i (other._i)
{
    // empty
}

inline FrameBuffer::ConstIterator &
FrameBuffer::ConstIterator::operator ++ ()
{
    ++_i;
    return *this;
}


inline FrameBuffer::ConstIterator
FrameBuffer::ConstIterator::operator ++ (int)
{
    ConstIterator tmp = *this;
    ++_i;
    return tmp;
}


inline const char *
FrameBuffer::ConstIterator::name () const
{
    return *_i->first;
}

inline const Slice &
FrameBuffer::ConstIterator::slice () const
{
    return _i->second;
}


inline bool
operator == (const FrameBuffer::ConstIterator &x,
             const FrameBuffer::ConstIterator &y)
{
    return x._i == y._i;
}


inline bool
operator != (const FrameBuffer::ConstIterator &x,
             const FrameBuffer::ConstIterator &y)
{
    return !(x == y);
}

} // namespace

#endif // MOXFILES_FRAMEBUFFER_H
