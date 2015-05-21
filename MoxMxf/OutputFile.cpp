/*
 *  OutputFile.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxMxf/OutputFile.h>

#include <MoxMxf/Dictionary.h>

#include <MoxMxf/Exception.h>

namespace MoxMxf
{

using namespace mxflib;  // because of _UL constants

OutputFile::OutputFile(IOStream &outfile, const EssenceList &essence, Rational EditRate, Position startTimeCode) :
	_essence(essence),
	_header_written(false),
	_duration(0)
{
	InitializeDict();
	
	if(essence.size() == 0)
		throw ArgExc("No tracks in essence");
		
	
	_fileH = RegisterIOStream(&outfile);
	
	_file = new mxflib::MXFFile;
	
	const bool opened = _file->OpenNewFromHandle(_fileH);
	
	if(!opened)
		throw IoExc("Error opening file");
	
	
	
	_metadata = new mxflib::Metadata();
	
	// {B658A3F9-280D-463c-9C12-FD6DAA6E1213}
	UInt8 Product_UL_Data[16] = { 0xb6, 0x58, 0xa3, 0xf9, 0x28, 0x0d, 0x46, 0x3c, 0x9c, 0x12, 0xfd, 0x6d, 0xaa, 0x6e, 0x12, 0x13 };
	mxflib::UUID Product_UL(Product_UL_Data);
	
	_identification = new mxflib::MDObject(Identification_UL);

	_identification->SetString(ProductName_UL, "fnord product");
	_identification->SetString(ProductVersion_UL, "fnord version");
	_identification->SetString(VersionString_UL, "fnord version string");
	_identification->SetString(ToolkitVersion_UL, mxflib::LibraryProductVersion());
	_identification->SetString(Platform_UL, "fnord platform");
	_identification->SetValue(ProductUID_UL, mxflib::DataChunk(16,Product_UL.GetValue()));

	_metadata->UpdateGenerations(_identification);
	
	_metadata->SetOP(MXFOP1a_UL);
		
	// Material Package
	mxflib::UMIDPtr materialUMID = mxflib::MakeUMID(4);
	mxflib::PackagePtr materialPkg = _metadata->AddMaterialPackage(materialUMID);
	_metadata->SetPrimaryPackage(materialPkg);
	
	// Source Package
	mxflib::UMIDPtr sourceUMID = mxflib::MakeUMID(4);
	mxflib::PackagePtr sourcePkg = _metadata->AddSourcePackage(sourceUMID);
	const bool essence_data_added = _metadata->AddEssenceContainerData(sourceUMID, _bodySID, _indexSID);
	assert(essence_data_added);
	
	
	const mxflib::Rational &edit_rate = EditRate;
	
	mxflib::TrackPtr timecodeTrack = materialPkg->AddTimecodeTrack(edit_rate);
	mxflib::TimecodeComponentPtr timecodeComponent = timecodeTrack->AddTimecodeComponent(startTimeCode);
	_duration_objs.push_back(SmartPtr_Cast(timecodeComponent, mxflib::Component));
	
	mxflib::TrackPtr timecodeTrack2 = sourcePkg->AddTimecodeTrack(edit_rate);
	mxflib::TimecodeComponentPtr timecodeComponent2 = timecodeTrack2->AddTimecodeComponent(startTimeCode);
	_duration_objs.push_back(SmartPtr_Cast(timecodeComponent2, mxflib::Component));
	
	
	// count how many of each ItemType we have
	std::map<UInt8, int> typecount;
	
	for(EssenceList::const_iterator e = essence.begin(); e != essence.end(); ++e)
	{
		const Descriptor *descriptor = e->second;
		
		typecount[ descriptor->getGCItemType() ] += 1;
	}
	
	std::map<UInt8, int> typenum; // we'll use this to count which number we're currently on
	
	
	mxflib::BodyStreamPtr bodyStream;
	
	std::vector<mxflib::MDObjectPtr> descriptor_objects;
	
	for(EssenceList::const_iterator e = essence.begin(); e != essence.end(); ++e)
	{
		const UInt32 track_number = e->first;
		const Descriptor *descriptor = e->second;
		
		_metadata->AddEssenceType( descriptor->getEssenceContainerLabel() );
		
		
		mxflib::TrackPtr source_track, material_track;
		
		if(const VideoDescriptor *videoDesc = dynamic_cast<const VideoDescriptor *>(descriptor))
		{
			assert(descriptor->getDescriptorType() == Descriptor::DescriptorTypeVideo);
			
			source_track = sourcePkg->AddPictureTrack(edit_rate);
			material_track = materialPkg->AddPictureTrack(edit_rate);
		}
		else if(const AudioDescriptor *audioDesc = dynamic_cast<const AudioDescriptor *>(descriptor))
		{
			assert(descriptor->getDescriptorType() == Descriptor::DescriptorTypeAudio);
		
			source_track = sourcePkg->AddSoundTrack(edit_rate);
			material_track = materialPkg->AddSoundTrack(edit_rate);
		}
		else
			assert(false);
		
		
		mxflib::SourceClipPtr source_clip = source_track->AddSourceClip();
		mxflib::SourceClipPtr material_clip = material_track->AddSourceClip();
		
		const bool linked = material_clip->MakeLink(source_track);
		assert(linked);
		
		_duration_objs.push_back(SmartPtr_Cast(source_clip, mxflib::Component));
		_duration_objs.push_back(SmartPtr_Cast(material_clip, mxflib::Component));
		
		
		mxflib::MDObjectPtr descriptor_obj = descriptor->makeDescriptorObj();
		
		assert(source_track->GetUInt(TrackID_UL) != 0);
		
		descriptor_obj->SetUInt(LinkedTrackID_UL, source_track->GetUInt(TrackID_UL));
		
		descriptor_objects.push_back(descriptor_obj);
		
		
		
		mxflib::EssenceSourcePtr essSource = new OutputEssenceSource(track_number, _output_buffer, edit_rate);
		
		essSource->SetDescriptor(descriptor_obj); // this doesn't really do anything but...
		
		assert(essSource->GetGCEssenceType() == descriptor->getGCItemType());
		assert(essSource->GetGCElementType() == descriptor->getGCElementType());
		
		if(!bodyStream)
		{
			bodyStream = new mxflib::BodyStream(_bodySID, essSource);
			
			bodyStream->AddIndexType(mxflib::BodyStream::StreamIndexFullFooter);
			
			bodyStream->SetIndexSID(_indexSID);
		}
		else
		{
			bodyStream->AddSubStream(essSource);
		}
		
		const UInt8 itemType = descriptor->getGCItemType();
		const UInt8 count = typecount[itemType];
		const UInt8 elementType = descriptor->getGCElementType();
		const UInt8 number = 1 + typenum[itemType]++; // +1 because that's the index mxflib starts at
		
		const UInt32 this_track_number = OutputFile::TrackNumber(itemType, count, elementType, number);
		
		source_track->SetUInt(TrackNumber_UL, this_track_number);
	}
	
	
	mxflib::MDObjectPtr main_descriptor = sourcePkg->AddChild(Descriptor_UL);
	
	if(descriptor_objects.size() == 1)
	{
		main_descriptor->MakeLink( descriptor_objects[0] );
	}
	else if(descriptor_objects.size() > 1)
	{
		mxflib::MDObjectPtr multi_descriptor = new mxflib::MDObject(MultipleDescriptor_UL);
		
		multi_descriptor->AddChild(SampleRate_UL)->SetInt("Numerator", EditRate.Numerator);
		multi_descriptor->AddChild(SampleRate_UL)->SetInt("Denominator", EditRate.Denominator);
		multi_descriptor->AddChild(EssenceContainer_UL)->SetValue(mxflib::DataChunk(16, mxflib::GCMulti_Data));

		mxflib::MDObjectPtr descriptor_array = multi_descriptor->AddChild(FileDescriptors_UL);
		
		for(int i=0; i < descriptor_objects.size(); i++)
		{
			descriptor_array->AddChild()->MakeLink( descriptor_objects[i] );
		}
		
		main_descriptor->MakeLink(multi_descriptor);
		
		_metadata->AddEssenceType(mxflib::GCMulti_Data);
	}
	else
		assert(false);
	
	
	
	_writer = new mxflib::BodyWriter(_file);
	
	_writer->SetKAG(512);
	_writer->SetForceBER4(true);
	
	_writer->AddStream(bodyStream);
	
	_index_manager = bodyStream->GetIndexManager();
	
	_index_manager->SetEditRate(EditRate);
}


OutputFile::~OutputFile()
{
	_writer->WriteBody();
	
	assert(_writer->BodyDone());

	_writer->EndPartition();
	
	
	for(std::deque<mxflib::ComponentPtr>::iterator i = _duration_objs.begin(); i != _duration_objs.end(); ++i)
	{
		mxflib::ComponentPtr source_clip = *i;
	
		source_clip->SetDuration(_duration);
	}
	
	_metadata->SetTime();
	
	_metadata->UpdateGenerations(_identification);
	
	
	mxflib::PartitionPtr footer_partition = new mxflib::Partition(FooterPartition_UL);
	
	initPartition(footer_partition, 0, _indexSID);
		
	footer_partition->AddMetadata(_metadata);
	
	_writer->SetPartition(footer_partition);
	
	_writer->WriteFooter(true);
	
	_writer->EndPartition();

	_file->Close();
	
	UnregisterIOStream(_fileH);
}

void
OutputFile::PushEssence(TrackNum trackNumber, mxflib::DataChunkPtr data, int KeyOffset, int TemporalOffset, int Flags)
{
	FrameInfo info(data, KeyOffset, TemporalOffset, Flags);

	bool pushed = false;
	
	for(OutBuffer::iterator i = _output_buffer.begin(); i != _output_buffer.end() && !pushed; ++i)
	{
		OutFrame &frame = *i;
		
		if( frame.find(trackNumber) == frame.end() )
		{
			frame[trackNumber] = info;
			
			pushed = true;
		}
	}
	
	if(!pushed)
	{
		OutFrame frame;
		
		frame[trackNumber] = info;
		
		_output_buffer.push_back(frame);
	}
	
	
	OutFrame &next_frame = _output_buffer.front();
	
	if(next_frame.size() == _essence.size())
	{
	#ifndef NDEBUG
		for(EssenceList::const_iterator i = _essence.begin(); i != _essence.end(); ++i)
		{
			const UInt32 &track_number = i->first;
			
			assert(next_frame.find(track_number) != next_frame.end());
		}
	#endif
	
		if(!_header_written)
		{
			mxflib::PartitionPtr header_partition = new mxflib::Partition(OpenHeader_UL);
			
			initPartition(header_partition, 0, 0);
			
			header_partition->AddMetadata(_metadata);
			
			_writer->SetPartition(header_partition);
			
			_writer->WriteHeader(false, false);
			
			_writer->EndPartition();
			
			mxflib::PartitionPtr body_partition = new mxflib::Partition(OpenHeader_UL);
			
			initPartition(body_partition, _bodySID, 0);
			
			_writer->SetPartition(body_partition);
			
			_header_written = true;
		}
		
		const Length frames_written = _writer->WritePartition(1, 0, false);
		
		assert(frames_written == 1);
		
		if(_index_manager)
		{
			const FrameInfo &next_frame_info = next_frame.begin()->second;
			
			if(next_frame_info.KeyOffset != 0)
				_index_manager->OfferKeyOffset(_duration, next_frame_info.KeyOffset);
			
			if(next_frame_info.TemporalOffset != 0)
				_index_manager->OfferTemporalOffset(_duration, next_frame_info.TemporalOffset);
			
			if(next_frame_info.Flags != -1)
				_index_manager->OfferFlags(_duration, next_frame_info.Flags);
		}
		else
			assert(false);

		_output_buffer.pop_front();
		
		_duration++;
	}
}


void
OutputFile::initPartition(mxflib::PartitionPtr partition, SID bodySID, SID indexSID)
{
	// shouldn't mxflib be doing this?
	partition->SetUInt(MajorVersion_UL, 1);
	partition->SetUInt(MinorVersion_UL, 2);
	partition->SetUInt(KAGSize_UL, _writer->GetKAG());
	partition->SetUInt(BodySID_UL, bodySID);
	partition->SetUInt(IndexSID_UL, indexSID);
	partition->SetValue(OperationalPattern_UL, _metadata->Child(OperationalPattern_UL)->GetData());
	
	mxflib::MDObjectPtr essence_containers = partition->AddChild(EssenceContainers_UL);
	
	mxflib::MDObjectPtr essence_containers_source = _metadata->Child(EssenceContainers_UL);
	
	if(essence_containers_source && essence_containers_source->GetClass() == mxflib::TYPEARRAY)
	{
		for(mxflib::MDObject::const_iterator o_itr = essence_containers_source->begin(); o_itr != essence_containers_source->end(); ++o_itr)
		{
			mxflib::MDObjectPtr container = o_itr->second;
			
			essence_containers->AddChild()->SetValue(container->GetData());
		}
	}
	else
		assert(false);
}


OutputFile::OutputEssenceSource::OutputEssenceSource(TrackNum trackNumber, const OutBuffer &outBuffer, mxflib::Rational EditRate) :
	_trackNumber(trackNumber),
	_outBuffer(outBuffer),
	_edit_rate(EditRate),
	_position(0)
{

}


size_t
OutputFile::OutputEssenceSource::GetEssenceDataSize(void)
{
	if(_outBuffer.size() == 0)
		throw LogicExc("No frames in output buffer");

	const OutFrame &next_frame = _outBuffer.front();
	
	OutFrame::const_iterator next_frame_part = next_frame.find(_trackNumber);
	
	if(next_frame_part == next_frame.end())
		throw LogicExc("Next frame not available");
	
	const FrameInfo &next_frame_info = next_frame_part->second;
	
	mxflib::DataChunkPtr next_frame_data = next_frame_info.data;
	
	if(!next_frame_data || next_frame_data->Size == 0)
		throw LogicExc("Don't have a next frame");
	
	return next_frame_data->Size;
}


mxflib::DataChunkPtr
OutputFile::OutputEssenceSource::GetEssenceData(size_t Size, size_t MaxSize)
{
	mxflib::DataChunkPtr data;
	
	if(_outBuffer.size() > 0)
	{
		const OutFrame &next_frame = _outBuffer.front();
		
		OutFrame::const_iterator next_frame_part = next_frame.find(_trackNumber);
		
		if(next_frame_part != next_frame.end())
		{
			const FrameInfo &next_frame_info = next_frame_part->second;
		
			mxflib::DataChunkPtr next_frame_data = next_frame_info.data;
			
			if( (Size == 0 || next_frame_data->Size < Size) && (MaxSize == 0 || next_frame_data->Size < MaxSize))
			{
				data = next_frame_data;
				
				_position++;
			}
		}
	}
	
	if(!data && _outBuffer.size() > 0)
	{
		assert(false); // we should not get called when there is no data in the queue
		
		data = new mxflib::DataChunk(0);
	}
	
	return data;
}


bool
OutputFile::OutputEssenceSource::EndOfData()
{
	if(_outBuffer.size() > 0)
	{
		const OutFrame &next_frame = _outBuffer.front();
		
		OutFrame::const_iterator next_frame_part = next_frame.find(_trackNumber);
		
		if(next_frame_part == next_frame.end())
		{
			return true;
		}
	}
	
	return false;
}

} // namespace
