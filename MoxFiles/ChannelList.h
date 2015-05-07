/*
 *  ChannelList.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/20/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_CHANNELLIST_H
#define MOXFILES_CHANNELLIST_H

#include <MoxFiles/PixelType.h>
#include <MoxFiles/Types.h>

#include <string>
#include <map>
#include <set>


namespace MoxFiles
{

struct Channel
{
    //------------------------------
    // Data type; see ImfPixelType.h
    //------------------------------

    PixelType		type;


    //--------------------------------------------
    // Subsampling: pixel (x, y) is present in the
    // channel only if 
    //
    //  x % xSampling == 0 && y % ySampling == 0
    //
    //--------------------------------------------

    int			xSampling;
    int			ySampling;


    //--------------------------------------------------------------
    // Hint to lossy compression methods that indicates whether
    // human perception of the quantity represented by this channel
    // is closer to linear or closer to logarithmic.  Compression
    // methods may optimize image quality by adjusting pixel data
    // quantization acording to this hint.
    // For example, perception of red, green, blue and luminance is
    // approximately logarithmic; the difference between 0.1 and 0.2
    // is perceived to be roughly the same as the difference between
    // 1.0 and 2.0.  Perception of chroma coordinates tends to be
    // closer to linear than logarithmic; the difference between 0.1
    // and 0.2 is perceived to be roughly the same as the difference
    // between 1.0 and 1.1.
    //--------------------------------------------------------------

    bool		pLinear;


    //------------
    // Constructor
    //------------
    
    Channel (PixelType type = UINT8,
	     int xSampling = 1,
	     int ySampling = 1,
	     bool pLinear = false);


    //------------
    // Operator ==
    //------------

    bool		operator == (const Channel &other) const;
};


class ChannelList
{
  public:

	size_t			size() const;
	
    //--------------
    // Add a channel
    //--------------

    void			insert (const char name[],
					const Channel &channel);

    void			insert (const std::string &name,
					const Channel &channel);

    //--------------
    // Remove a channel
    //--------------

    void			erase (const char name[]);

    void			erase (const std::string &name);
					
    //------------------------------------------------------------------
    // Access to existing channels:
    //
    // [n]		Returns a reference to the channel with name n.
    //			If no channel with name n exists, an IEX_NAMESPACE::ArgExc
    //			is thrown.
    //
    // findChannel(n)	Returns a pointer to the channel with name n,
    //			or 0 if no channel with name n exists.
    //
    //------------------------------------------------------------------

    Channel &			operator [] (const char name[]);
    const Channel &		operator [] (const char name[]) const;

    Channel &			operator [] (const std::string &name);
    const Channel &		operator [] (const std::string &name) const;

    Channel *			findChannel (const char name[]);
    const Channel *		findChannel (const char name[]) const;

    Channel *			findChannel (const std::string &name);
    const Channel *		findChannel (const std::string &name) const;


    //-------------------------------------------
    // Iterator-style access to existing channels
    //-------------------------------------------

    typedef std::map <Name, Channel> ChannelMap;

    class Iterator;
    class ConstIterator;

    Iterator			begin ();
    ConstIterator		begin () const;

    Iterator			end ();
    ConstIterator		end () const;

    Iterator			find (const char name[]);
    ConstIterator		find (const char name[]) const;

    Iterator			find (const std::string &name);
    ConstIterator		find (const std::string &name) const;

    
    //-----------------------------------------------------------------
    // Support for image layers:
    //
    // In an image file with many channels it is sometimes useful to
    // group the channels into "layers", that is, into sets of channels
    // that logically belong together.  Grouping channels into layers
    // is done using a naming convention:  channel C in layer L is
    // called "L.C".
    //
    // For example, a computer graphic image may contain separate
    // R, G and B channels for light that originated at each of
    // several different virtual light sources.  The channels in
    // this image might be called "light1.R", "light1.G", "light1.B",
    // "light2.R", "light2.G", "light2.B", etc.
    // 
    // Note that this naming convention allows layers to be nested;
    // for example, "light1.specular.R" identifies the "R" channel
    // in the "specular" sub-layer of layer "light1".
    //
    // Channel names that don't contain a "." or that contain a
    // "." only at the beginning or at the end are not considered
    // to be part of any layer.
    //
    // layers(lns)		sorts the channels in this ChannelList
    //				into layers and stores the names of
    //				all layers, sorted alphabetically,
    //				into string set lns.
    //
    // channelsInLayer(ln,f,l)	stores a pair of iterators in f and l
    // 				such that the loop
    //
    // 				for (ConstIterator i = f; i != l; ++i)
    // 				   ...
    //
    //				iterates over all channels in layer ln.
    //				channelsInLayer (ln, l, p) calls
    //				channelsWithPrefix (ln + ".", l, p).
    //
    //-----------------------------------------------------------------

    void		layers (std::set <std::string> &layerNames) const;

    void		channelsInLayer (const std::string &layerName,
	    				 Iterator &first,
					 Iterator &last);

    void		channelsInLayer (const std::string &layerName,
	    				 ConstIterator &first,
					 ConstIterator &last) const;


    //-------------------------------------------------------------------
    // Find all channels whose name begins with a given prefix:
    //
    // channelsWithPrefix(p,f,l) stores a pair of iterators in f and l
    // such that the following loop iterates over all channels whose name
    // begins with string p:
    //
    //		for (ConstIterator i = f; i != l; ++i)
    //		    ...
    //
    //-------------------------------------------------------------------

    void			channelsWithPrefix (const char prefix[],
						    Iterator &first,
						    Iterator &last);

    void			channelsWithPrefix (const char prefix[],
						    ConstIterator &first,
						    ConstIterator &last) const;

    void			channelsWithPrefix (const std::string &prefix,
						    Iterator &first,
						    Iterator &last);

    void			channelsWithPrefix (const std::string &prefix,
						    ConstIterator &first,
						    ConstIterator &last) const;

    //------------
    // Operator ==
    //------------

    bool			operator == (const ChannelList &other) const;

  private:

    ChannelMap			_map;
};


//----------
// Iterators
//----------

class ChannelList::Iterator
{
  public:

    Iterator ();
    Iterator (const ChannelList::ChannelMap::iterator &i);

    Iterator &			operator ++ ();
    Iterator 			operator ++ (int);

    const char *		name () const;
    Channel &			channel () const;

  private:

    friend class ChannelList::ConstIterator;

    ChannelList::ChannelMap::iterator _i;
};


class ChannelList::ConstIterator
{
  public:

    ConstIterator ();
    ConstIterator (const ChannelList::ChannelMap::const_iterator &i);
    ConstIterator (const ChannelList::Iterator &other);

    ConstIterator &		operator ++ ();
    ConstIterator 		operator ++ (int);

    const char *		name () const;
    const Channel &		channel () const;

  private:

    friend bool operator == (const ConstIterator &, const ConstIterator &);
    friend bool operator != (const ConstIterator &, const ConstIterator &);

    ChannelList::ChannelMap::const_iterator _i;
};


//-----------------
// Inline Functions
//-----------------

inline
ChannelList::Iterator::Iterator (): _i()
{
    // empty
}


inline
ChannelList::Iterator::Iterator (const ChannelList::ChannelMap::iterator &i):
    _i (i)
{
    // empty
}


inline ChannelList::Iterator &		
ChannelList::Iterator::operator ++ ()
{
    ++_i;
    return *this;
}


inline ChannelList::Iterator 	
ChannelList::Iterator::operator ++ (int)
{
    Iterator tmp = *this;
    ++_i;
    return tmp;
}


inline const char *
ChannelList::Iterator::name () const
{
    return *_i->first;
}


inline Channel &	
ChannelList::Iterator::channel () const
{
    return _i->second;
}


inline
ChannelList::ConstIterator::ConstIterator (): _i()
{
    // empty
}

inline
ChannelList::ConstIterator::ConstIterator
    (const ChannelList::ChannelMap::const_iterator &i): _i (i)
{
    // empty
}


inline
ChannelList::ConstIterator::ConstIterator (const ChannelList::Iterator &other):
    _i (other._i)
{
    // empty
}

inline ChannelList::ConstIterator &
ChannelList::ConstIterator::operator ++ ()
{
    ++_i;
    return *this;
}


inline ChannelList::ConstIterator 		
ChannelList::ConstIterator::operator ++ (int)
{
    ConstIterator tmp = *this;
    ++_i;
    return tmp;
}


inline const char *
ChannelList::ConstIterator::name () const
{
    return *_i->first;
}

inline const Channel &	
ChannelList::ConstIterator::channel () const
{
    return _i->second;
}


inline bool
operator == (const ChannelList::ConstIterator &x,
	     const ChannelList::ConstIterator &y)
{
    return x._i == y._i;
}


inline bool
operator != (const ChannelList::ConstIterator &x,
	     const ChannelList::ConstIterator &y)
{
    return !(x == y);
}


} // namespace

#endif
