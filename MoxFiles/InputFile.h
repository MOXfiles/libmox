/*
 *  InputFile.h
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_INPUTFILE_H
#define MOXFILES_INPUTFILE_H

#include <MoxFiles/Header.h>

#include <MoxMxf/InputFile.h>


namespace MoxFiles
{
	class InputFile
	{
	  public:
		InputFile(MoxMxf::IOStream &infile);
		virtual ~InputFile();
		
		const Header & header() const { return _header; }
		
	  private:
		MoxMxf::InputFile _mxf_file;
		Header _header;
	};

} // namespace

#endif // MOXFILES_INPUTFILE_H