/*
 *  Types.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/20/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_TYPES_H
#define MOXFILES_TYPES_H

#include <MoxMxf/Types.h>
#include <MoxMxf/IOStream.h>

#include <ImathBox.h>
#include <ImfName.h>
#include <ImfInt64.h>

namespace MoxFiles
{
	using MoxMxf::Rational;

	using MoxMxf::RefCount;
	using MoxMxf::SmartPtr;
	using MoxMxf::DataChunk;
	using MoxMxf::DataChunkPtr;
	
	using MoxMxf::IOStream;
	
	using MoxMxf::UInt64;
	using MoxMxf::UInt32;
	using MoxMxf::UInt16;
	using MoxMxf::UInt8;
	using MoxMxf::Int64;
	using MoxMxf::Int32;
	using MoxMxf::Int16;
	using MoxMxf::Int8;
	
	//using Imath::V2s;
	using Imath::V2i;
	//using Imath::V2f;
	//using Imath::V2d;
	//using Imath::V3s;
	//using Imath::V3i;
	//using Imath::V3f;
	//using Imath::V3d;
	//using Imath::V4s;
	//using Imath::V4i;
	//using Imath::V4f;
	//using Imath::V4d;

	//using Imath::Box2s;
	using Imath::Box2i;
	//using Imath::Box2f;
	//using Imath::Box2d;
	//using Imath::Box3s;
	//using Imath::Box3i;
	//using Imath::Box3f;
	//using Imath::Box3d;
	
	using Imf::Name;

} // namespace

#endif // MOXFILES_TYPES_H
