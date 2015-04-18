/*
 *  Track.h
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXMXF_TRACK_H
#define MOXMXF_TRACK_H

#include <MoxMxf/Types.h>

#include <MoxMxf/Descriptor.h>

#include <mxflib/mxflib.h>

namespace MoxMxf
{
	class Track
	{
	  public:
		Track(mxflib::TrackPtr track);
		virtual ~Track() {}
		
		typedef enum TrackType					// copied from Track::TrackType in metadata.h
		{
			TrackTypeUndetermined = -1,			//!< Not yet checked
			TrackTypeUnknown = 0,				//!< Not a known type
			TrackTypeTimecode,					//!< Timecode track (of any type)
			TrackTypeDescriptiveMetadata,		//!< Descriptive Metadata track
			TrackTypePictureEssence,			//!< Picture track
			TrackTypeSoundEssence,				//!< Sound track
			TrackTypeDataEssence,				//!< Data track
			TrackTypeAuxiliary,					//!< Auxiliary track
			TrackTypeParsedText,				//!< Parsed Text track
		};
		
		TrackType getType() const { return _type; }
		Position getOrigin() const { return _origin; }
		Rational getEditRate() const { return _editrate; }
		
		virtual Length getDuration() const = 0;

	  private:
		TrackType _type;
		Position _origin;
		Rational _editrate;
	};

	class TimecodeTrack : public Track
	{
	  public:
		TimecodeTrack(mxflib::TrackPtr track);
		virtual ~TimecodeTrack() {}

		virtual Length getDuration() const { return _duration; }
		
		Position getStartTimecode() const { return _start_timecode; }
		
	  private:
		Length _duration;
		Position _start_timecode;
		UInt32 _rounded_timecode_base;
		bool _drop_frame;
	};

	class SourceTrack : public Track
	{
	  public:
		SourceTrack(mxflib::TrackPtr track);
		virtual ~SourceTrack();

		virtual Length getDuration() const { return _duration; }
		
		TrackNum getNumber() const { return _number; }
		SID getBodySID() const { return _bodySID; }
		SID getIndexSID() const { return _indexSID; }
		
		Descriptor *getDescriptor() const { return _descriptor; }
		
	  protected:
		mxflib::MDObjectPtr getDescriptorObj() const { return _track_descriptor; }
		
		Descriptor *_descriptor;

	  private:
		Length _duration;
		TrackNum _number;
		SID _bodySID;
		SID _indexSID;

	  private:
		mxflib::MDObjectPtr _track_descriptor;
	};

	class PictureTrack : public SourceTrack
	{
	  public:
		PictureTrack(mxflib::TrackPtr track);
		virtual ~PictureTrack() {}

	  //private:
		//UInt32 _width;
		//UInt32 _height;
		//UInt8 _depth;
	};

	class SoundTrack : public SourceTrack
	{
	  public:
		SoundTrack(mxflib::TrackPtr track);
		virtual ~SoundTrack() {}

	  //private:
		//UInt32 _channels;
		//Uint32 _bit_depth;
		//Rational _sample_rate;
	};

} // namespace


#endif // MOXMXF_TRACK_H