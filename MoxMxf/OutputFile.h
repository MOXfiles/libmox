/*
 *  OutputFile.h
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXMXF_OUTPUTFILE_H
#define MOXMXF_OUTPUTFILE_H

#include <MoxMxf/IOStream.h>

#include <MoxMxf/Descriptor.h>

#include <mxflib/mxflib.h>

#include <deque>

namespace MoxMxf
{
	class OutputFile
	{
	  public:
		typedef std::map<TrackNum, const Descriptor *> EssenceList;
	  
		OutputFile(IOStream &outfile, const EssenceList &essence, Rational EditRate, Position startTimeCode);
		~OutputFile();
		
		void PushEssence(TrackNum trackNumber, mxflib::DataChunkPtr data, int KeyOffset=0, int TemporalOffset=0, int Flags=-1);

	  public:
		static inline TrackNum TrackNumber(UInt8 itemType, UInt8 count, UInt8 elementType, UInt8 number)
		{
			return (itemType << 24) | (count << 16) | (elementType << 8) | number;
		}
		
		//static inline UInt8 ItemType(TrackNum trackNumber) { return ((trackNumber & 0xff000000) >> 24); }
		//static inline UInt8 ElementType(TrackNum trackNumber) { return ((trackNumber & 0x0000ff00) >> 8); }
		
	  private:
		void initPartition(mxflib::PartitionPtr partition, SID bodySID, SID indexSID);

	  private:
		FileHandle _fileH;
		mxflib::MXFFilePtr _file;
		mxflib::BodyWriterPtr _writer;
		
		const EssenceList _essence;
		
		mxflib::MetadataPtr _metadata;
		mxflib::MDObjectPtr _identification;
		bool _header_written;
		
		Position _duration;
		std::deque<mxflib::ComponentPtr> _duration_objs;
		
		typedef struct FrameInfo {
			mxflib::DataChunkPtr data;
			int KeyOffset;
			int TemporalOffset;
			int Flags;
			
			FrameInfo(mxflib::DataChunkPtr d=NULL, int k=0, int t=0, int f=-1) : data(d), KeyOffset(k), TemporalOffset(t), Flags(f) {}
		} FrameInfo;
		
		typedef std::map<TrackNum, FrameInfo> OutFrame;
		typedef std::deque<OutFrame> OutBuffer;
		
		OutBuffer _output_buffer;
		mxflib::IndexManagerPtr _index_manager;
		
		static const SID _bodySID = 1;
		static const SID _indexSID = 2;
		
		class OutputEssenceSource : public mxflib::EssenceSource
		{
		  public:
			OutputEssenceSource(TrackNum trackNumber, const OutBuffer &outBuffer, mxflib::Rational EditRate);
			virtual ~OutputEssenceSource() {}
			
			virtual size_t GetEssenceDataSize(void);
			virtual mxflib::DataChunkPtr GetEssenceData(size_t Size = 0, size_t MaxSize = 0);
			virtual bool EndOfItem(void) { return true; } // frame-wrapped, so always provide an entire wrapping unit
			virtual bool EndOfData(void);
			virtual UInt8 GetGCEssenceType(void) { return (_trackNumber & 0xff000000) >> 24; }
			virtual UInt8 GetGCElementType(void) { return (_trackNumber & 0x0000ff00) >> 8; }
			virtual mxflib::Rational GetEditRate(void) { return _edit_rate; }
			virtual Position GetCurrentPosition(void) { return _position; }
			
		  private:
			const TrackNum _trackNumber;
			const OutBuffer &_outBuffer;
			const mxflib::Rational _edit_rate;
			Position _position;
		};
	};

} // namespace


#endif