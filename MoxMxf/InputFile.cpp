/*
 *  InputFile.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxMxf/InputFile.h>

#include <MoxMxf/Dictionary.h>

#include <MoxMxf/Exception.h>

namespace MoxMxf
{

using namespace mxflib;

mxflib::DataChunk &
FramePart::getData()
{
	mxflib::DataChunk &data = _obj->GetData();
	
	assert(_obj->GetLength() > 0);
	
	if(data.Size == 0 || data.Data == NULL)
	{
		const size_t size = _obj->ReadData();
		
		assert(size > 0);
		assert(data.Size == size);
		assert(data.Data != NULL);
	}
	
	return data;
}


InputFile::InputFile(IOStream &infile)
{
	InitializeDict();

	_fileH = RegisterIOStream(&infile);

	_file = new mxflib::MXFFile;
	
	const bool opened = _file->OpenFromHandle(_fileH);
	
	if(!opened)
		throw IoExc("Error opening file");
		
	
	mxflib::PartitionPtr master_partition = _file->ReadMasterPartition();
	
	if(master_partition)
	{
		if( master_partition->ReadMetadata() )
		{
			mxflib::MetadataPtr mdata = master_partition->ParseMetadata();
			
			if(mdata)
			{
				mxflib::PackagePtr primary_package = mdata->GetPrimaryPackage();
				
				if(primary_package)
				{
					assert(*primary_package->GetUL() == MaterialPackage_UL);
					
					for(mxflib::TrackList::iterator t = primary_package->Tracks.begin(); t != primary_package->Tracks.end(); ++t)
					{
						mxflib::TrackPtr track = *t;
						
						const Int32 track_id = track->GetInt(TrackID_UL);
						
						assert(_tracks.find(track_id) == _tracks.end());
						
						const Track::TrackType type = static_cast<Track::TrackType>(track->GetTrackType());
						
						if(type == Track::TrackTypeTimecode)
						{
							_tracks[track_id] = new TimecodeTrack(track);
						}
						else if(type == Track::TrackTypePictureEssence)
						{
							_tracks[track_id] = new PictureTrack(track);
						}
						else if(type == Track::TrackTypeSoundEssence)
						{
							_tracks[track_id] = new SoundTrack(track);
						}
						else
							assert(false); // unexpected track type
					}
				}
				else
					throw InputExc("No primary package???");
			}
			else
				throw InputExc("Couldn't parse metadata");
		}
		else
			throw InputExc("Couldn't read metadata");
	}
	else
		throw InputExc("Couldn't get master partition");
	
	
	if( _file->GetRIP() )
	{
		for(mxflib::RIP::iterator p = _file->FileRIP.begin(); p != _file->FileRIP.end(); ++p)
		{
			mxflib::PartitionInfoPtr p_info = p->second;
			
			assert(p->first == p_info->ByteOffset);
			
			_file->Seek(p_info->ByteOffset);
			
			mxflib::PartitionPtr partition = _file->ReadPartition();
			
			if(partition)
			{
				const SID bodySID = partition->GetUInt(BodySID_UL);
				const SID indexSID = partition->GetUInt(IndexSID_UL);
				
				assert(bodySID == p_info->GetBodySID());
				assert(indexSID == p_info->GetIndexSID() || !p_info->SIDsKnown());
				
				if(indexSID != 0)
				{
					if(_index_map.find(indexSID) == _index_map.end())
					{
						_index_map[ indexSID ] = new mxflib::IndexTable;
					}
					
					mxflib::IndexTablePtr Table = _index_map[ indexSID ];
					
					mxflib::MDObjectListPtr segments = partition->ReadIndex();
					
					if(segments)
					{
						assert(partition->GetInt64(IndexByteCount_UL) > 0);
						
						for(mxflib::MDObjectList::iterator it = segments->begin(); it != segments->end(); ++it)
						{
							Table->AddSegment(*it);
						}
					}
				}
			}
			else
				assert(false); // didn't see that coming
		}
	}
	else
		throw InputExc("Couldn't get RIP");
}


InputFile::~InputFile()
{
	_file->Close();
	
	for(TrackMap::iterator i = _tracks.begin(); i != _tracks.end(); ++i)
	{
		delete i->second;
		
		i->second = NULL;
	}
	
	UnregisterIOStream(_fileH);
}


Length
InputFile::getDuration() const
{
	Length duration = 0;

#ifdef NDEBUG
	// cut to the chase when not debugging
	for(TrackMap::const_iterator t = _tracks.begin(); t != _tracks.end(); ++t)
	{
		duration = t->second->getDuration();
	
		if(duration > 0)
			return duration
	}
#else
	// lots of asserts for the debug case
	for(TrackMap::const_iterator t = _tracks.begin(); t != _tracks.end(); ++t)
	{
		if(duration == 0)
		{
			duration = t->second->getDuration();
		}
		else
		{
			assert(duration == t->second->getDuration());
		}
		
		if(SourceTrack *source = dynamic_cast<SourceTrack *>(t->second))
		{
			const SID indexSID = source->getIndexSID();
		
			if(indexSID != 0)
			{
				IndexMap::const_iterator idx = _index_map.find(indexSID);
			
				if(idx != _index_map.end())
				{
					mxflib::IndexTablePtr table = idx->second;
					
					assert(table->GetDuration() == duration);
					//assert(table->EditRate == source->getEditRate());
				}
				else
					assert(false);
			}
		}
	}
	
	assert(duration > 0);
#endif // NDEBUG
	
	return duration;
}


class SequentialReadHandler : public mxflib::GCReadHandler_Base
{
  public:
	SequentialReadHandler(Frame::FrameParts &frameparts) : _frameparts(frameparts) {}
	virtual ~SequentialReadHandler() {}
	
	bool HandleData(mxflib::GCReaderPtr Caller, mxflib::KLVObjectPtr Object)
	{
		const mxflib::GCElementKind kind = Object->GetGCElementKind();
		
		if(kind.IsValid)
		{
			assert(_frameparts.find( Object->GetGCTrackNumber() ) == _frameparts.end());
		
			_frameparts[ Object->GetGCTrackNumber() ] = new FramePart(Object);
		}
		else
		{
			const Length len = Object->GetLength();
			
			if(len > 0)
			{
				mxflib::DataChunk &data = Object->GetData();
				
				const size_t size = Object->ReadData();
				
				//const char *buf = (const char *)data.Data;
				
				assert(size == len);
				assert(data.Size == size);
				assert(data.Data != NULL);
			}
		}
		
		return true;
		
	}

  private:
	Frame::FrameParts &_frameparts;
};


FramePtr
InputFile::getFrame(Position EditUnit, SID bodySID, SID indexSID)
{
	FramePtr the_frame = new Frame;
	
	Frame::FrameParts &frameparts = the_frame->getFrameParts();
	
	
	// how many frame parts in a frame?
	std::map<TrackNum, SourceTrack *> source_tracks;
	
	for(TrackMap::const_iterator t = _tracks.begin(); t != _tracks.end(); ++t)
	{
		if(SourceTrack *source = dynamic_cast<SourceTrack *>(t->second))
		{
			source_tracks[ source->getNumber() ] = source;
		}
	}
	
	const int frame_parts = source_tracks.size();
	
	
	mxflib::BodyReaderPtr reader = new mxflib::BodyReader(_file);
	
	if(reader)
	{
		mxflib::GCReadHandlerPtr handler = new SequentialReadHandler(frameparts);
		
		const bool made = reader->MakeGCReader(bodySID, handler);
		
		if(made)
		{
			IndexMap::const_iterator idx = _index_map.find(indexSID);
			
			if(idx != _index_map.end())
			{
				mxflib::IndexTablePtr index = idx->second;
				
				assert(index->BodySID == bodySID);
			
				mxflib::IndexPosPtr posPtr = index->Lookup(EditUnit);
				
				if(posPtr)
				{
					assert(posPtr->Exact);
				
					the_frame->setKeyOffset( posPtr->KeyFrameOffset );
					the_frame->setTemporalOffset( posPtr->TemporalOffset );
					the_frame->setFlags( posPtr->Flags );
				
					reader->Seek(bodySID, posPtr->Location);
				}
				else
				{
					assert(false); // asked for a frame not in the index
				
					return getFrame(EditUnit, bodySID, 0);
				}
			}
			else
			{
				throw NoImplExc("Non-index seek unimplemented.");
			}
			
			
			while(frameparts.size() < frame_parts)
			{
				const bool success = reader->ReadFromFile(true);
				
				assert(success);
			}
			
		#ifndef NDEBUG
			// see if we got all the frameparts we expected
			for(Frame::FrameParts::const_iterator fp = frameparts.begin(); fp != frameparts.end(); ++fp)
			{
				std::map<TrackNum, SourceTrack *>::iterator tnum = source_tracks.find(fp->first);
				
				if(tnum != source_tracks.end())
				{
					source_tracks.erase(tnum);
				}
				else
					assert(false);
			}
			
			assert(source_tracks.size() == 0);
		#endif
		}
		else
			throw LogicExc("Couldn't make GCReader");
		
	}
	else
		throw LogicExc("Couldn't create reader");
			
	
	return the_frame;
}


mxflib::PackagePtr
InputFile::findPackage(mxflib::MetadataParent mdata, const mxflib::UMID &package_id)
{
	for(mxflib::PackageList::const_iterator i = mdata->Packages.begin(); i != mdata->Packages.end(); ++i)
	{
		mxflib::PackagePtr pack = *i;
		
		mxflib::UMID this_id(pack->Child(PackageUID_UL)->GetData().Data);
		
		if(this_id == package_id)
			return pack;
	}
	
	return NULL;
}

UInt32
InputFile::getSID(mxflib::MetadataParent mdata, const mxflib::UMID &package_id, bool getIndexSID)
{
	mxflib::MDObjectPtr content_storage_ref = mdata->Child(ContentStorageObject_UL);

	if(content_storage_ref && content_storage_ref->GetRef())
	{
		mxflib::MDObjectPtr content_storage = content_storage_ref->GetRef();
	
		mxflib::MDObjectPtr essence_data_objects = content_storage->Child(EssenceDataObjects_UL);
		
		for(mxflib::MDObject::const_iterator o_itr = essence_data_objects->begin(); o_itr != essence_data_objects->end(); ++o_itr)
		{
			mxflib::MDObjectPtr essence_data_ref = o_itr->second;
			
			mxflib::MDObjectPtr essence_data = essence_data_ref->GetRef();
			
			mxflib::UMID this_id(essence_data->Child(LinkedPackageUID_UL)->GetData().Data);
			
			if(this_id == package_id)
			{
				if(getIndexSID)
					return essence_data->GetUInt(IndexSID_UL);
				else
					return essence_data->GetUInt(BodySID_UL);
			}
		}
	}
	
	return 0;
}

} // namespace