/*
 *  FrameBuffer.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/21/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_AUDIOBUFFER_H
#define MOXFILES_AUDIOBUFFER_H

#include <MoxFiles/SampleType.h>

#include <MoxFiles/Types.h>

#include <string>
#include <map>
#include <list>

#include <stddef.h>

namespace MoxFiles
{


struct AudioSlice
{
    SampleType           type;


    char *              base;
    ptrdiff_t           stride;


    AudioSlice (SampleType type = SIGNED24,
           char * base = 0,
           ptrdiff_t stride = 0);
};


class AudioBuffer : public RefCount<AudioBuffer>
{
  public:
	AudioBuffer(UInt64 length);
	AudioBuffer(const AudioBuffer &other);
	~AudioBuffer() {}
	
	void fastForward(Int64 samples);
	void fastForward(const char name[], Int64 samples);
	void fastForward(const std::string &name, Int64 samples);
	
	void rewind();
	
	void readFromBuffer(AudioBuffer &other, UInt64 samples = 0, bool fillMissing = false); // will more &other playheads forward
	void copyFromBuffer(const AudioBuffer &other, UInt64 samples = 0, bool fillMissing = false); // will not
	
	void fillRemaining();
	void fillRemaining(const char name[]);
	void fillRemaining(const std::string &name);
	
	void attachData(DataChunkPtr dat) { _data.push_back(dat); }
	
	
	UInt64 length() const { return _length; } // number of samples
	
	size_t size() const { return _map.size(); } // number of channels

    //------------
    // Add a slice
    //------------

    void                        insert (const char name[],
                                        const AudioSlice &slice);

    void                        insert (const std::string &name,
                                        const AudioSlice &slice);

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

    AudioSlice &                     operator [] (const char name[]);
    const AudioSlice &               operator [] (const char name[]) const;

    AudioSlice &                     operator [] (const std::string &name);
    const AudioSlice &               operator [] (const std::string &name) const;

    AudioSlice *                     findSlice (const char name[]);
    const AudioSlice *               findSlice (const char name[]) const;

    AudioSlice *                     findSlice (const std::string &name);
    const AudioSlice *               findSlice (const std::string &name) const;
	
	// these slices are moved ahead based on current playheads
	AudioSlice						playheadSlice(const char name[]) const;
	AudioSlice						playheadSlice(const std::string &name) const;

	UInt64							remaining() const;
	UInt64							remaining(const char name[]) const;
	UInt64							remaining(const std::string &name) const;

    //-----------------------------------------
    // Iterator-style access to existing slices
    //-----------------------------------------

    typedef std::map <Name, AudioSlice> SliceMap;

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
	
	const UInt64				_length;
	
	typedef std::map<Name, UInt64> ReadHeads;
	
	ReadHeads					_readHeads;
	
	std::list<DataChunkPtr>		_data;
};

typedef SmartPtr<AudioBuffer> AudioBufferPtr;


AudioBufferPtr MakeAudioBufferType(const AudioBuffer &buffer, SampleType type);
	


//----------
// Iterators
//----------

class AudioBuffer::Iterator
{
  public:

    Iterator ();
    Iterator (const AudioBuffer::SliceMap::iterator &i);

    Iterator &                  operator ++ ();
    Iterator                    operator ++ (int);

    const char *                name () const;
    AudioSlice &                slice () const;

  private:

    friend class AudioBuffer::ConstIterator;

    AudioBuffer::SliceMap::iterator _i;
};


class AudioBuffer::ConstIterator
{
  public:

    ConstIterator ();
    ConstIterator (const AudioBuffer::SliceMap::const_iterator &i);
    ConstIterator (const AudioBuffer::Iterator &other);

    ConstIterator &             operator ++ ();
    ConstIterator               operator ++ (int);

    const char *                name () const;
    const AudioSlice &          slice () const;

  private:

    friend bool operator == (const ConstIterator &, const ConstIterator &);
    friend bool operator != (const ConstIterator &, const ConstIterator &);

    AudioBuffer::SliceMap::const_iterator _i;
};


//-----------------
// Inline Functions
//-----------------

inline
AudioBuffer::Iterator::Iterator (): _i()
{
    // empty
}


inline
AudioBuffer::Iterator::Iterator (const AudioBuffer::SliceMap::iterator &i):
    _i (i)
{
    // empty
}


inline AudioBuffer::Iterator &
AudioBuffer::Iterator::operator ++ ()
{
    ++_i;
    return *this;
}


inline AudioBuffer::Iterator
AudioBuffer::Iterator::operator ++ (int)
{
    Iterator tmp = *this;
    ++_i;
    return tmp;
}


inline const char *
AudioBuffer::Iterator::name () const
{
    return *_i->first;
}


inline AudioSlice &
AudioBuffer::Iterator::slice () const
{
    return _i->second;
}


inline
AudioBuffer::ConstIterator::ConstIterator (): _i()
{
    // empty
}

inline
AudioBuffer::ConstIterator::ConstIterator
    (const AudioBuffer::SliceMap::const_iterator &i): _i (i)
{
    // empty
}


inline
AudioBuffer::ConstIterator::ConstIterator (const AudioBuffer::Iterator &other):
    _i (other._i)
{
    // empty
}

inline AudioBuffer::ConstIterator &
AudioBuffer::ConstIterator::operator ++ ()
{
    ++_i;
    return *this;
}


inline AudioBuffer::ConstIterator
AudioBuffer::ConstIterator::operator ++ (int)
{
    ConstIterator tmp = *this;
    ++_i;
    return tmp;
}


inline const char *
AudioBuffer::ConstIterator::name () const
{
    return *_i->first;
}

inline const AudioSlice &
AudioBuffer::ConstIterator::slice () const
{
    return _i->second;
}


inline bool
operator == (const AudioBuffer::ConstIterator &x,
             const AudioBuffer::ConstIterator &y)
{
    return x._i == y._i;
}


inline bool
operator != (const AudioBuffer::ConstIterator &x,
             const AudioBuffer::ConstIterator &y)
{
    return !(x == y);
}

} // namespace

#endif // MOXFILES_AUDIOBUFFER_H
