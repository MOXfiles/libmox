/*
 *  mxflib_messages.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */


#include <stdarg.h>
#include <stdio.h>

namespace mxflib
{

//#ifndef NDEBUG
void debug(const char *Fmt, ...)
{
	va_list args;

	va_start(args, Fmt);
	printf("mxflib debug: ");
	vprintf(Fmt, args);
	va_end(args);
}
//#endif

void warning(const char *Fmt, ...)
{
	va_list args;

	va_start(args, Fmt);
	printf("mxflib Warning: ");
	vprintf(Fmt, args);
	va_end(args);
}

void error(const char *Fmt, ...)
{
	va_list args;

	va_start(args, Fmt);
	printf("MXFLIB ERROR: ");
	vprintf(Fmt, args);
	va_end(args);
}

} // namespace
