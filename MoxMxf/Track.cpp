/*
 *  Track.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxMxf/Track.h>

//#include <MoxMxf/Descriptor.h>
#include <MoxMxf/InputFile.h>

#include <MoxMxf/Exception.h>


namespace MoxMxf
{

using namespace mxflib; // didn't feel like adding mxflib:: to all those _UL constants

Track::Track(mxflib::TrackPtr track)
{
	_type = static_cast<TrackType>(track->GetTrackType());
	_origin = track->GetInt64(Origin_UL);
	_editrate = Rational(track->Child(EditRate_UL)->GetInt("Numerator"), track->Child(EditRate_UL)->GetInt("Denominator"));
}


TimecodeTrack::TimecodeTrack(mxflib::TrackPtr track) :
	Track(track)
{
	assert(getType() == TrackTypeTimecode);

	if(track->Components.size() >= 1)
	{
		assert(track->Components.size() == 1);
		
		mxflib::ComponentPtr comp = *track->Components.begin();
	
		mxflib::TimecodeComponentPtr timecode_component = TimecodeComponent::GetTimecodeComponent(comp->Object);
		
		if(timecode_component)
		{
			_duration = timecode_component->GetUInt64(ComponentLength_UL);
			_start_timecode = timecode_component->GetInt64(StartTimecode_UL);
			_rounded_timecode_base = timecode_component->GetUInt(RoundedTimecodeBase_UL);
			_drop_frame = timecode_component->GetInt(DropFrame_UL);
		}
		else
			throw TypeExc("Couldn't get timecode component");
	}
	else
		throw InputExc("No component");
}


SourceTrack::SourceTrack(mxflib::TrackPtr track) :
	Track(track),
	_descriptor(NULL)
{
	assert(track->GetInt(TrackNumber_UL) == 0);

	if(track->Components.size() >= 1)
	{
		assert(track->Components.size() == 1);
		
		mxflib::ComponentPtr comp = *track->Components.begin();
	
		mxflib::SourceClipPtr source_clip = mxflib::SourceClip::GetSourceClip(comp->Object);
		
		if(source_clip)
		{
			_duration = source_clip->GetUInt64(ComponentLength_UL);
			
			assert(_duration != 0);
			assert(getOrigin() == source_clip->GetUInt64(StartPosition_UL));
		
			const UInt32 source_track_id = source_clip->GetInt(SourceTrackID_UL);
			
			assert(source_track_id == track->GetInt(TrackID_UL));
			
			const mxflib::UMID source_package_id(source_clip->Child(SourcePackageID_UL)->GetData().Data);
			
			_bodySID = InputFile::getSID(track->GetParent()->GetParent(), source_package_id, false);
			_indexSID = InputFile::getSID(track->GetParent()->GetParent(), source_package_id, true);
			
			assert(_bodySID != 0);
			
			mxflib::PackagePtr source_package = InputFile::findPackage(track->GetParent()->GetParent(), source_package_id);
			
			if(source_package)
			{
				mxflib::TrackPtr source_track = NULL;
				
				for(mxflib::TrackList::iterator t = source_package->Tracks.begin(); t != source_package->Tracks.end() && !source_track; ++t)
				{
					mxflib::TrackPtr this_track = *t;
					
					const Int32 this_track_id = this_track->GetInt(TrackID_UL);
					
					if(this_track_id == source_track_id)
						source_track = this_track;
				}
				
				if(source_track)
				{
					_number = source_track->GetInt(TrackNumber_UL);
					
					assert(_number != 0);
					assert(getOrigin() == source_track->GetInt64(Origin_UL));
					assert(getEditRate() == Rational(source_track->Child(EditRate_UL)->GetInt("Numerator"), source_track->Child(EditRate_UL)->GetInt("Denominator")));
				}
				else
					throw InputExc("Couldn't get source track");
				
				
				mxflib::MDObjectPtr descriptor_ref = source_package->Child(Descriptor_UL);
				
				if(descriptor_ref)
				{
					mxflib::MDObjectPtr descriptor = descriptor_ref->GetRef();
					
					if(*descriptor->GetUL() == MultipleDescriptor_UL)
					{
						assert(descriptor->IsA(MultipleDescriptor_UL)); // better way to do that?
						
						mxflib::MDObjectPtr descriptor_ref_array = descriptor->Child(FileDescriptors_UL);
						
						if(descriptor_ref_array)
						{
							for(mxflib::MDObject::const_iterator o_itr = descriptor_ref_array->begin(); o_itr != descriptor_ref_array->end() && !_track_descriptor; ++o_itr)
							{
								mxflib::MDObjectPtr descriptor_i = o_itr->second->GetRef();
								
								if(descriptor_i->GetInt(LinkedTrackID_UL) == source_track_id)
									_track_descriptor = descriptor_i;
							}
						}
					}
					else if(descriptor->GetInt(LinkedTrackID_UL) == source_track_id)
					{
						_track_descriptor = descriptor;
					}
				}
				else
					throw InputExc("No descriptor");
				
				
				if(!_track_descriptor)
					throw InputExc("No track descriptor");
			}
			else
				throw InputExc("Source package not available");
		}
		else
			throw TypeExc("Couldn't get source clip");
	}
	else
		throw InputExc("No component");
}

SourceTrack::~SourceTrack()
{
	delete _descriptor;
}


PictureTrack::PictureTrack(mxflib::TrackPtr track) :
	SourceTrack(track)
{
	assert(getType() == TrackTypePictureEssence);
	
	mxflib::MDObjectPtr descriptor = getDescriptorObj();
	
	if(descriptor)
	{
		if(*descriptor->GetUL() == MPEG2VideoDescriptor_UL)
		{
			_descriptor = new MPEGDescriptor(descriptor);
		
			//_width = descriptor->Child(DisplayWidth_UL)->GetUInt();
			//_height = descriptor->Child(DisplayHeight_UL)->GetUInt();
			//_depth = descriptor->Child(ComponentDepth_UL)->GetUInt();
			
			//assert(_width == descriptor->Child(SampledWidth_UL)->GetUInt());
			//assert(_width == descriptor->Child(StoredWidth_UL)->GetUInt());
			//assert(_height == descriptor->Child(SampledHeight_UL)->GetUInt());
			//assert(_height == descriptor->Child(StoredHeight_UL)->GetUInt());
			
			//assert(getEditRate() == Rational(descriptor->Child(SampleRate_UL)->GetInt("Numerator"), descriptor->Child(SampleRate_UL)->GetInt("Denominator")));
		}
	}
	else
		throw InputExc("No descriptor");
}


SoundTrack::SoundTrack(mxflib::TrackPtr track) :
	SourceTrack(track)
{
	assert(getType() == TrackTypeSoundEssence);
	
	mxflib::MDObjectPtr descriptor = getDescriptorObj();
	
	if(descriptor)
	{
		if(*descriptor->GetUL() == AES3PCMDescriptor_UL)
		{
			_descriptor = new AES3Descriptor(descriptor);
		
			//_channels = descriptor->Child(ChannelCount_UL)->GetUInt();
			//_bit_depth = descriptor->Child(QuantizationBits_UL)->GetUInt();
			//_sample_rate = Rational(descriptor->Child(AudioSamplingRate_UL)->GetInt("Numerator"), descriptor->Child(AudioSamplingRate_UL)->GetInt("Denominator"));
			
			//assert(getEditRate() == Rational(descriptor->Child(SampleRate_UL)->GetInt("Numerator"), descriptor->Child(SampleRate_UL)->GetInt("Denominator")));
		}
	}
	else
		throw InputExc("No descriptor");
}

} // namespace