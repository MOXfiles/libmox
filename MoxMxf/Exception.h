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

// our exceptions are going to be the OpenEXR exceptions

#undef ASSERT // to kill a warning when mxflib/system.h is included before and this gets redefines

#include <Iex.h>


namespace MoxMxf
{
	typedef Iex::BaseExc		BaseExc;
	typedef Iex::ArgExc			ArgExc;
	typedef Iex::LogicExc		LogicExc;
	typedef Iex::InputExc		InputExc;
	typedef Iex::IoExc			IoExc;
	typedef Iex::MathExc		MathExc;
	typedef Iex::ErrnoExc		ErrnoExc;
	typedef Iex::NoImplExc		NoImplExc;
	typedef Iex::NullExc		NullExc;
	typedef Iex::TypeExc		TypeExc;
} // namespace

#endif // MOXMXF_EXCEPTION_H