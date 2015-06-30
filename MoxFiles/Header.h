/*
 *  Header.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/20/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_HEADER_H
#define MOXFILES_HEADER_H

//#include <MoxFiles/Types.h>

#include <MoxFiles/Attribute.h>

#include <map>

namespace MoxFiles
{

class Header
{
  public:
	Header(const Box2i &dataWindow,
			const Box2i &displayWindow,
			const Rational &frameRate = Rational(24, 1),
			const Rational &sampleRate = Rational(0, 1),
			VideoCompression videoCompression = PNG,
			AudioCompression audioCompression = PCM,
			int duration = 0,
			Int64 audioDuration = 0);

	Header(int width = 64,
			int height = 64,
			const Rational &frameRate = Rational(24, 1),
			const Rational &sampleRate = Rational(0, 1),
			VideoCompression videoCompression = UNCOMPRESSED,
			AudioCompression audioCompression = PCM,
			int duration = 0,
			Int64 audioDuration = 0);
			
	Header(const Header &other);
	
	~Header();
	
	Header & operator = (const Header &other);

     //---------------------------------------------------------------
    // Add an attribute:
    //
    // insert(n,attr)	If no attribute with name n exists, a new
    //			attribute with name n, and the same type as
    //			attr, is added, and the value of attr is
    //			copied into the new attribute.
    //
    //			If an attribute with name n exists, and its
    //			type is the same as attr, the value of attr
    //			is copied into this attribute.
    //
    //			If an attribute with name n exists, and its
    //			type is different from attr, an IEX_NAMESPACE::TypeExc
    //			is thrown.
    //
    //---------------------------------------------------------------

    void			insert (const char name[],
				        const Attribute &attribute);

    void			insert (const std::string &name,
				        const Attribute &attribute);

    //---------------------------------------------------------------
    // Remove an attribute:
    //
    // remove(n)       If an attribute with name n exists, then it
    //                 is removed from the map of present attributes.
    //
    //                 If no attribute with name n exists, then this
    //                 functions becomes a 'no-op'
    //
    //---------------------------------------------------------------
    void                        erase (const char name[]);
    void                        erase (const std::string &name);

    
    
    //------------------------------------------------------------------
    // Access to existing attributes:
    //
    // [n]			Returns a reference to the attribute
    //				with name n.  If no attribute with
    //				name n exists, an IEX_NAMESPACE::ArgExc is thrown.
    //
    // typedAttribute<T>(n)	Returns a reference to the attribute
    //				with name n and type T.  If no attribute
    //				with name n exists, an IEX_NAMESPACE::ArgExc is
    //				thrown.  If an attribute with name n
    //				exists, but its type is not T, an
    //				IEX_NAMESPACE::TypeExc is thrown.
    //
    // findTypedAttribute<T>(n)	Returns a pointer to the attribute with
    //				name n and type T, or 0 if no attribute
    //				with name n and type T exists.
    //
    //------------------------------------------------------------------

    Attribute &			operator [] (const char name[]);
    const Attribute &		operator [] (const char name[]) const;

    Attribute &			operator [] (const std::string &name);
    const Attribute &		operator [] (const std::string &name) const;

    template <class T> T&	typedAttribute (const char name[]);
    template <class T> const T&	typedAttribute (const char name[]) const;

    template <class T> T&	typedAttribute (const std::string &name);
    template <class T> const T&	typedAttribute (const std::string &name) const;

    template <class T> T*	findTypedAttribute (const char name[]);
    template <class T> const T*	findTypedAttribute (const char name[]) const;

    template <class T> T*	findTypedAttribute (const std::string &name);
    template <class T> const T*	findTypedAttribute (const std::string &name)
								       const;

    //---------------------------------------------
    // Iterator-style access to existing attributes
    //---------------------------------------------

    typedef std::map <Name, Attribute *> AttributeMap;

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


    //--------------------------------
    // Access to predefined attributes
    //--------------------------------

    Box2i &			dataWindow ();
    const Box2i &	dataWindow () const;

    Box2i &			displayWindow ();
    const Box2i &	displayWindow () const;
	
	int width () const;
	int height () const;
	
	int &				duration();
	const int &			duration() const;

	Int64 &				audioDuration();
	const Int64 &		audioDuration() const;
	
    Rational &			pixelAspectRatio ();
    const Rational &	pixelAspectRatio () const;

    Rational &			frameRate ();
    const Rational &	frameRate () const;
	
    Rational &			sampleRate ();
    const Rational &	sampleRate () const;
	
    ChannelList &		channels ();
    const ChannelList &	channels () const;
	
    AudioChannelList &		audioChannels ();
    const AudioChannelList & audioChannels () const;
	
	VideoCompression &			videoCompression ();
	const VideoCompression &	videoCompression () const;
	
 	AudioCompression &			audioCompression ();
	const AudioCompression &	audioCompression () const;
	
    //-------------------------------------------------------------
    // Sanity check -- examines the header, and throws an exception
    // if it finds something wrong (empty display window, negative
    // pixel aspect ratio, unknown compression sceme etc.)
    //
    // set isTiled to true if you are checking a tiled/multi-res
    // header
    //-------------------------------------------------------------

    void			sanityCheck () const;


  private:
	AttributeMap	_map;

};


//----------
// Iterators
//----------

class Header::Iterator
{
  public:

    Iterator ();
    Iterator (const Header::AttributeMap::iterator &i);

    Iterator &			operator ++ ();
    Iterator 			operator ++ (int);

    const char *		name () const;
    Attribute &			attribute () const;

  private:

    friend class Header::ConstIterator;

    Header::AttributeMap::iterator _i;
};


class Header::ConstIterator
{
  public:

    ConstIterator ();
    ConstIterator (const Header::AttributeMap::const_iterator &i);
    ConstIterator (const Header::Iterator &other);

    ConstIterator &		operator ++ ();
    ConstIterator 		operator ++ (int);

    const char *		name () const;
    const Attribute &		attribute () const;

  private:

    friend bool operator == (const ConstIterator &, const ConstIterator &);
    friend bool operator != (const ConstIterator &, const ConstIterator &);

    Header::AttributeMap::const_iterator _i;
};


//------------------------------------------------------------------------
// Library initialization:
//
// In a multithreaded program, staticInitialize() must be called once
// during startup, before the program accesses any other functions or
// classes in the IlmImf library.  Calling staticInitialize() in this
// way avoids races during initialization of the library's global
// variables.
//
// Single-threaded programs are not required to call staticInitialize();
// initialization of the library's global variables happens automatically.
//
//------------------------------------------------------------------------

void staticInitialize();
	

//-----------------
// Inline Functions
//-----------------


inline
Header::Iterator::Iterator (): _i()
{
    // empty
}


inline
Header::Iterator::Iterator (const Header::AttributeMap::iterator &i): _i (i)
{
    // empty
}


inline Header::Iterator &		
Header::Iterator::operator ++ ()
{
    ++_i;
    return *this;
}


inline Header::Iterator 	
Header::Iterator::operator ++ (int)
{
    Iterator tmp = *this;
    ++_i;
    return tmp;
}


inline const char *
Header::Iterator::name () const
{
    return *_i->first;
}


inline Attribute &	
Header::Iterator::attribute () const
{
    return *_i->second;
}


inline
Header::ConstIterator::ConstIterator (): _i()
{
    // empty
}

inline
Header::ConstIterator::ConstIterator
    (const Header::AttributeMap::const_iterator &i): _i (i)
{
    // empty
}


inline
Header::ConstIterator::ConstIterator (const Header::Iterator &other):
    _i (other._i)
{
    // empty
}

inline Header::ConstIterator &
Header::ConstIterator::operator ++ ()
{
    ++_i;
    return *this;
}


inline Header::ConstIterator 		
Header::ConstIterator::operator ++ (int)
{
    ConstIterator tmp = *this;
    ++_i;
    return tmp;
}


inline const char *
Header::ConstIterator::name () const
{
    return *_i->first;
}


inline const Attribute &	
Header::ConstIterator::attribute () const
{
    return *_i->second;
}


inline bool
operator == (const Header::ConstIterator &x, const Header::ConstIterator &y)
{
    return x._i == y._i;
}


inline bool
operator != (const Header::ConstIterator &x, const Header::ConstIterator &y)
{
    return !(x == y);
}


//---------------------
// Template definitions
//---------------------

template <class T>
T &
Header::typedAttribute (const char name[])
{
    Attribute *attr = &(*this)[name];
    T *tattr = dynamic_cast <T*> (attr);

    if (tattr == 0)
	throw MoxMxf::TypeExc ("Unexpected attribute type.");

    return *tattr;
}


template <class T>
const T &
Header::typedAttribute (const char name[]) const
{
    const Attribute *attr = &(*this)[name];
    const T *tattr = dynamic_cast <const T*> (attr);

    if (tattr == 0)
	throw MoxMxf::TypeExc ("Unexpected attribute type.");

    return *tattr;
}


template <class T>
T &
Header::typedAttribute (const std::string &name)
{
    return typedAttribute<T> (name.c_str());
}


template <class T>
const T &
Header::typedAttribute (const std::string &name) const
{
    return typedAttribute<T> (name.c_str());
}


template <class T>
T *
Header::findTypedAttribute (const char name[])
{
    AttributeMap::iterator i = _map.find (name);
    return (i == _map.end())? 0: dynamic_cast <T*> (i->second);
}


template <class T>
const T *
Header::findTypedAttribute (const char name[]) const
{
    AttributeMap::const_iterator i = _map.find (name);
    return (i == _map.end())? 0: dynamic_cast <const T*> (i->second);
}


template <class T>
T *
Header::findTypedAttribute (const std::string &name)
{
    return findTypedAttribute<T> (name.c_str());
}


template <class T>
const T *
Header::findTypedAttribute (const std::string &name) const
{
    return findTypedAttribute<T> (name.c_str());
}


} // namespace


#endif // MOXFILES_HEADER_H