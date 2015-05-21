/*
 *  InputFile.h
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXMXF_INPUTFILE_H
#define MOXMXF_INPUTFILE_H

#include <MoxMxf/IOStream.h>

//#include <MoxMxf/Types.h>
#include <MoxMxf/Track.h>

#include <mxflib/mxflib.h>

#include <map>

namespace MoxMxf
{
	class FramePart : public mxflib::RefCount<FramePart>
	{
	  public:
		FramePart(mxflib::KLVObjectPtr obj) : _obj(obj) {}
		~FramePart() {}
		
		mxflib::DataChunk & getData();
		Length getDataSize() { return _obj->GetLength(); } // GetLength() should be const

	  private:
		mxflib::KLVObjectPtr _obj;
	};

	typedef mxflib::SmartPtr<FramePart> FramePartPtr;

	class Frame : public mxflib::RefCount<Frame>
	{
	  public:
		Frame() : _keyOffset(0), _temporalOffset(0), _flags(-1) {}
		~Frame() {}
		
		typedef std::map<UInt32, FramePartPtr> FrameParts; // frame _number, part
		
		FrameParts & getFrameParts() { return _frameparts; }

		void setKeyOffset(int k) { _keyOffset = k; }
		void setTemporalOffset(int t) { _temporalOffset = t; }
		void setFlags(int f) { _flags = f; }
		
		int getKeyOffset() const { return _keyOffset; }
		int getTemporalOffset() const { return _temporalOffset; }
		int getFlags() const { return _flags; }
		
	  private:
		FrameParts _frameparts;
		int _keyOffset;
		int _temporalOffset;
		int _flags;
	};

	typedef mxflib::SmartPtr<Frame> FramePtr;


	class InputFile
	{
	  public:
		InputFile(IOStream &infile);
		~InputFile();

		typedef std::map<TrackNum, Track *> TrackMap;
		
		const TrackMap & getTracks() const { return _tracks; }
		
		Length getDuration() const;
		Rational getEditRate() const;
		
		FramePtr getFrame(Position EditUnit, SID bodySID, SID indexSID);
		
		static mxflib::PackagePtr findPackage(mxflib::MetadataParent mdata, const mxflib::UMID &package_id);
		static UInt32 getSID(mxflib::MetadataParent mdata, const mxflib::UMID &package_id, bool getIndexSID);
		
	  private:
		FileHandle _fileH;
		mxflib::MXFFilePtr _file;
		
		TrackMap _tracks;
		
		typedef std::map<UInt32, mxflib::IndexTablePtr> IndexMap;
		IndexMap _index_map;
	};

} // namespace

#endif // MOXMXF_INPUTFILE_H