/*
 *  Exception.h
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXMXF_EXCEPTION_H
#define MOXMXF_EXCEPTION_H

// Made in the model of Iex from OpenEXR

#include <exception>

#include <string>

namespace MoxMxf
{
	class BaseExc : public std::exception
	{
	  public:
		BaseExc(const std::string &s) throw() : _what(s) {}
		virtual ~BaseExc() throw() {}
		
		virtual const char* what() const throw() { return _what.c_str(); }
	
	  private:
		std::string _what;
	};


#define MOX_DEFINE_EXC_EXP(name, base)                          \
    class name : public base                                    \
    {                                                           \
      public:                                                   \
        name (const std::string &text) throw(): base (text) {}  \
        virtual ~name() throw() { }                             \
    };
	
	MOX_DEFINE_EXC_EXP(ArgExc, BaseExc);		// Invalid arguments to a function call
	MOX_DEFINE_EXC_EXP(LogicExc, BaseExc);		// General error in a program's logic
	MOX_DEFINE_EXC_EXP(InputExc, BaseExc);		// Invalid input data, e.g. from a file
	MOX_DEFINE_EXC_EXP(IoExc, BaseExc);			// Input or output operation failed
	MOX_DEFINE_EXC_EXP(NoImplExc, BaseExc);		// Missing method exception, A reminder to lazy software people to get back to work
	MOX_DEFINE_EXC_EXP(NullExc, BaseExc);		// A pointer is inappropriately null
	MOX_DEFINE_EXC_EXP(TypeExc, BaseExc);		// An object is an inappropriate type
	
} // namespace

#endif // MOXMXF_EXCEPTION_H
