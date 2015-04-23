/*
 *  InputFile.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/InputFile.h>


namespace MoxFiles
{

InputFile::InputFile(MoxMxf::IOStream &infile) :
	_mxf_file(infile)
{

}


InputFile::~InputFile()
{

}

} // namespace