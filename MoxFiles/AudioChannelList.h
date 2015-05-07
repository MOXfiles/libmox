/*
 *  AudioChannelList.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/29/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_AUDIOCHANNELLIST_H
#define MOXFILES_AUDIOCHANNELLIST_H

#include <MoxFiles/SampleType.h>
#include <MoxFiles/Types.h>

#include <string>
#include <map>

namespace MoxFiles
{

struct AudioChannel
{
	SampleType	type;
	

    //------------
    // Constructor
    //------------

	AudioChannel(SampleType type = SIGNED24);
	
	
	bool		operator == (const AudioChannel &other) const;
};


class AudioChannelList
{
  public:

	size_t			size() const;
	
    //--------------
    // Add a channel
    //--------------

    void			insert (const char name[],
					const AudioChannel &channel);

    void			insert (const std::string &name,
					const AudioChannel &channel);

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

    AudioChannel &			operator [] (const char name[]);
    const AudioChannel &	operator [] (const char name[]) const;

    AudioChannel &			operator [] (const std::string &name);
    const AudioChannel &	operator [] (const std::string &name) const;

    AudioChannel *			findChannel (const char name[]);
    const AudioChannel *	findChannel (const char name[]) const;

    AudioChannel *			findChannel (const std::string &name);
    const AudioChannel *	findChannel (const std::string &name) const;


    //-------------------------------------------
    // Iterator-style access to existing channels
    //-------------------------------------------

    typedef std::map <Name, AudioChannel> AudioChannelMap;

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

    


    //------------
    // Operator ==
    //------------

    bool			operator == (const AudioChannelList &other) const;

  private:

    AudioChannelMap		_map;
};


class AudioChannelList::Iterator
{
  public:

    Iterator ();
    Iterator (const AudioChannelList::AudioChannelMap::iterator &i);

    Iterator &			operator ++ ();
    Iterator 			operator ++ (int);

    const char *		name () const;
    AudioChannel &		channel () const;

  private:

    friend class AudioChannelList::ConstIterator;

    AudioChannelList::AudioChannelMap::iterator _i;
};


class AudioChannelList::ConstIterator
{
  public:

    ConstIterator ();
    ConstIterator (const AudioChannelList::AudioChannelMap::const_iterator &i);
    ConstIterator (const AudioChannelList::Iterator &other);

    ConstIterator &		operator ++ ();
    ConstIterator 		operator ++ (int);

    const char *		name () const;
    const AudioChannel &channel () const;

  private:

    friend bool operator == (const ConstIterator &, const ConstIterator &);
    friend bool operator != (const ConstIterator &, const ConstIterator &);

    AudioChannelList::AudioChannelMap::const_iterator _i;
};


//-----------------
// Inline Functions
//-----------------

inline
AudioChannelList::Iterator::Iterator (): _i()
{
    // empty
}


inline
AudioChannelList::Iterator::Iterator (const AudioChannelList::AudioChannelMap::iterator &i):
    _i (i)
{
    // empty
}


inline AudioChannelList::Iterator &		
AudioChannelList::Iterator::operator ++ ()
{
    ++_i;
    return *this;
}


inline AudioChannelList::Iterator 	
AudioChannelList::Iterator::operator ++ (int)
{
    Iterator tmp = *this;
    ++_i;
    return tmp;
}


inline const char *
AudioChannelList::Iterator::name () const
{
    return *_i->first;
}


inline AudioChannel &	
AudioChannelList::Iterator::channel () const
{
    return _i->second;
}


inline
AudioChannelList::ConstIterator::ConstIterator (): _i()
{
    // empty
}

inline
AudioChannelList::ConstIterator::ConstIterator
    (const AudioChannelList::AudioChannelMap::const_iterator &i): _i (i)
{
    // empty
}


inline
AudioChannelList::ConstIterator::ConstIterator (const AudioChannelList::Iterator &other):
    _i (other._i)
{
    // empty
}

inline AudioChannelList::ConstIterator &
AudioChannelList::ConstIterator::operator ++ ()
{
    ++_i;
    return *this;
}


inline AudioChannelList::ConstIterator 		
AudioChannelList::ConstIterator::operator ++ (int)
{
    ConstIterator tmp = *this;
    ++_i;
    return tmp;
}


inline const char *
AudioChannelList::ConstIterator::name () const
{
    return *_i->first;
}

inline const AudioChannel &	
AudioChannelList::ConstIterator::channel () const
{
    return _i->second;
}


inline bool
operator == (const AudioChannelList::ConstIterator &x,
	     const AudioChannelList::ConstIterator &y)
{
    return x._i == y._i;
}


inline bool
operator != (const AudioChannelList::ConstIterator &x,
	     const AudioChannelList::ConstIterator &y)
{
    return !(x == y);
}



} // namespace

#endif // MOXFILES_AUDIOCHANNELLIST_H