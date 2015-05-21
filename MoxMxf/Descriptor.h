/*
 *  Descriptor.h
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXMXF_DESCRIPTOR_H
#define MOXMXF_DESCRIPTOR_H

#include <MoxMxf/Types.h>

#include <mxflib/mxflib.h>

#include <vector>

namespace MoxMxf
{
	class Descriptor
	{
	  public:
		Descriptor(mxflib::MDObjectPtr descriptor);
		Descriptor(Rational sample_rate);
		Descriptor(const Descriptor &other);
		virtual ~Descriptor() {}
		
		virtual mxflib::MDObjectPtr makeDescriptorObj() const;
		
		virtual UInt8 getGCItemType() const = 0;
		virtual UInt8 getGCElementType() const = 0;
		
		typedef enum DescriptorType
		{
			DescriptorTypeVideo,
			DescriptorTypeAudio
		};
		
		virtual DescriptorType getDescriptorType() const = 0;
		
		const mxflib::UL & getEssenceContainerLabel() const { return _essence_container; }
		
		const Rational & getSampleRate() const { return _sample_rate; } // edit rate, not audio sampling rate
		
		void setContainerDuration(Length duration) { _container_duration = duration; }

	  protected:
		void setEssenceContainerLabel(const mxflib::UL &ul) { _essence_container = ul; }
		void setCodec(const mxflib::UL &ul) { _codec = ul; }
	  
		virtual const mxflib::UL & getDescriptorUL() const = 0;
		
	  private:
		// SMPTE 377M D.1 File descriptor
		Rational _sample_rate;
		Length _container_duration;
		mxflib::UL _essence_container;
		mxflib::UL _codec;
	};

	class VideoDescriptor : public Descriptor
	{
	  public:
		VideoDescriptor(mxflib::MDObjectPtr descriptor);
		VideoDescriptor(Rational sample_rate, UInt32 width, UInt32 height);
		VideoDescriptor(const VideoDescriptor &other);
		virtual ~VideoDescriptor() {}
		
		virtual mxflib::MDObjectPtr makeDescriptorObj() const;
		
		virtual UInt8 getGCItemType() const = 0;
		virtual UInt8 getGCElementType() const = 0;
		
		DescriptorType getDescriptorType() const { return DescriptorTypeVideo; }
		
		typedef enum VideoCodec
		{
			VideoCodecUncompressedCDCI,
			VideoCodecUncompressedRGB,
			VideoCodecMPEG2,
			VideoCodecUnknown
		};
		
		virtual VideoCodec getVideoCodec() const = 0;
		
		enum {
			LayoutType_FullFrame = 0,
			LayoutType_SeparateFields = 1,
			LayoutType_OneField = 2,
			LayoutType_MixedFields = 3,
			LayoutType_SegmentedFrame = 4,
			LayoutType_UnknownLayout = 255
		};
		
		void setFrameLayout(UInt8 layout) { _frame_layout = layout; }
		
		UInt32 getWidth() const { return _stored_width; }
		UInt32 getHeight() const { return _stored_height; }
		
	  protected:
		void setPictureEssenceCoding(const mxflib::UL &ul) { _picture_essence_coding = ul; }
	  
		virtual const mxflib::UL & getDescriptorUL() const = 0;
		
	  private:
		// SMPTE 377M D.2.1 Generic picture essence descriptor
		UInt8 _signal_standard;
		UInt8 _frame_layout;
		UInt32 _stored_width;
		UInt32 _stored_height;
		Int32 _stored_f2_offset;
		UInt32 _sampled_width;
		UInt32 _sampled_height;
		Int32 _sampled_x_offset;
		Int32 _sampled_y_offset;
		UInt32 _display_width;
		UInt32 _display_height;
		Int32 _display_x_offset;
		Int32 _display_y_offset;
		Int32 _display_f2_offset;
		Rational _aspect_ratio;
		UInt8 _active_format_descriptor;
		std::vector<Int32> _video_line_map;
		UInt8 _alpha_transparency;
		mxflib::UL _capture_gamma; // i.e. Transfer Characteristic
		UInt32 _image_alignment_offset;
		UInt32 _image_start_offset;
		UInt32 _image_end_offset;
		UInt8 _field_dominance;
		mxflib::UL _picture_essence_coding;
	};

	class CDCIDescriptor : public VideoDescriptor
	{ // "Color-Difference Component Image Descriptor"
	  public:
		CDCIDescriptor(mxflib::MDObjectPtr descriptor);
		CDCIDescriptor(Rational sample_rate, UInt32 width, UInt32 height, UInt32 horizontal_subsampling, UInt32 vertical_subsampling);
		CDCIDescriptor(const CDCIDescriptor &other);
		virtual ~CDCIDescriptor() {}
		
		virtual mxflib::MDObjectPtr makeDescriptorObj() const;
		
		virtual UInt8 getGCItemType() const { return 0x15; } // SMPTE 384M-2005 7.1 (0x15 == "GC Picture")
		virtual UInt8 getGCElementType() const { return 0x02; }; // frame-wrapped
		
		virtual VideoCodec getVideoCodec() const { return VideoCodecUncompressedCDCI; }
		
		enum {
			ColorSiting_CoSiting = 0,
			ColorSiting_MidPoint = 1,
			ColorSiting_ThreeTap = 2,
			ColorSiting_Quincunx = 3,
			ColorSiting_Rec601 = 4,
			ColorSiting_Unknown = 255
		};
		
	  protected:
		virtual const mxflib::UL & getDescriptorUL() const { return mxflib::CDCIEssenceDescriptor_UL; }
		
	  private:
		// SMPTE 377M D.2.2 CDCI (Color-difference component image) picture essence descriptor
		UInt32 _component_depth;
		UInt32 _horizontal_subsampling;
		UInt32 _vertical_subsampling;
		UInt8 _color_siting;
		bool _reversed_byte_order;
		Int16 _padding_bits;
		UInt32 _alpha_sample_depth;
		UInt32 _black_ref_level;
		UInt32 _white_ref_level;
		UInt32 _color_range;
	};
	
	class RGBADescriptor : public VideoDescriptor
	{
	  public:
		RGBADescriptor(mxflib::MDObjectPtr descriptor);
		RGBADescriptor(Rational sample_rate, UInt32 width, UInt32 height);
		RGBADescriptor(const RGBADescriptor &other);
		virtual ~RGBADescriptor() {}
		
		virtual mxflib::MDObjectPtr makeDescriptorObj() const;
		
		virtual UInt8 getGCItemType() const { return 0x15; } // SMPTE 384M-2005 7.1 (0x15 == "GC Picture")
		virtual UInt8 getGCElementType() const { return 0x02; }; // frame-wrapped
		
		virtual VideoCodec getVideoCodec() const { return VideoCodecUncompressedRGB; }
		
		// see SMPTE 377M E.2.46 (page 99)
		typedef struct RGBALayoutItem
		{
			UInt8 code;
			UInt8 depth;
			
			RGBALayoutItem(UInt8 c = 0, UInt8 d = 0) : code(c), depth(d) {}
		} RGBALayoutItem;
		
		// mxflib handles padding this out to its fixed 8-item size
		typedef std::vector<RGBALayoutItem> RGBALayout;
		
		void setPixelLayout(const RGBALayout &layout) { _pixel_layout = layout; }
		const RGBALayout & getPixelLayout() { return _pixel_layout; }
		
	  protected:
		virtual const mxflib::UL & getDescriptorUL() const { return mxflib::RGBAEssenceDescriptor_UL; }
	
	  private:
		enum {
			ScanningDir_LRTB = 0, // left to right, top to bottom
			ScanningDir_RLTB = 1,
			ScanningDir_LRBT = 2,
			ScanningDir_RLBT = 3,
			ScanningDir_TBLR = 4,
			ScanningDir_TBRL = 5,
			ScanningDir_BTLR = 6,
			ScanningDir_BTRL = 7
		};
		
		UInt32 _component_max_ref;
		UInt32 _component_min_ref;
		UInt32 _alpha_max_ref;
		UInt32 _alpha_min_ref;
		UInt8 _scanning_direction;
		RGBALayout _pixel_layout;
		// palette
		// palette layout
	};


	class MPEGDescriptor : public CDCIDescriptor
	{
	  public:
		MPEGDescriptor(mxflib::MDObjectPtr descriptor);
		MPEGDescriptor(Rational sample_rate, UInt32 width, UInt32 height, UInt32 horizontal_subsampling, UInt32 vertical_subsampling);
		MPEGDescriptor(const MPEGDescriptor &other);
		virtual ~MPEGDescriptor() {}
		
		virtual mxflib::MDObjectPtr makeDescriptorObj() const;
		
		virtual UInt8 getGCItemType() const { return 0x15; } // SMPTE ST 381-2:2011 8.1.1 (0x15 == "GC Picture")
		virtual UInt8 getGCElementType() const { return 0x05; } // frame-wrapped MPEG
		
		virtual VideoCodec getVideoCodec() const { return VideoCodecMPEG2; }
		
		enum {
			CodedType_Unknown = 0,
			CodedType_Progressive = 1,
			CodedType_Interlaced = 2,
			CodedType_Mixed = 3
		};
		
	  protected:
		virtual const mxflib::UL & getDescriptorUL() const { return mxflib::MPEG2VideoDescriptor_UL; }
		
	  private:
		// SMPTE ST 381-2:2011 10.2 MPEG Video Sescriptor
		bool _single_sequence;
		bool _constant_b_picture;
		UInt8 _coded_content_type;
		bool _low_delay;
		bool _closed_GOP;
		bool _identical_GOP;
		UInt16 _maximum_GOP_size;
		UInt16 _maximum_b_picture_count;
		UInt32 _bit_rate;
		UInt8 _profile_and_level;
	};
	

	class AudioDescriptor : public Descriptor
	{
	  public:
		AudioDescriptor(mxflib::MDObjectPtr descriptor);
		AudioDescriptor(Rational sample_rate, Rational audio_sampling_rate, UInt32 channel_count, UInt32 quantization_bits);
		AudioDescriptor(const AudioDescriptor &other);
		virtual ~AudioDescriptor() {}
		
		virtual mxflib::MDObjectPtr makeDescriptorObj() const;
		
		virtual UInt8 getGCItemType() const = 0;
		virtual UInt8 getGCElementType() const = 0;
		
		virtual DescriptorType getDescriptorType() const { return DescriptorTypeAudio; }
		
		typedef enum AudioCodec
		{
			AudioCodecUncompressedPCM,
			AudioCodecAES3,
			AudioCodecUnknown
		};
		
		virtual AudioCodec getAudioCodec() const = 0;
		
		const Rational & getAudioSamplingRate() const { return _audio_sampling_rate; }
		UInt32 getChannelCount() const { return _channel_count; }
		UInt32 getBitDepth() const { return _quantization_bits; }
		
	  protected:
		void setSoundCompression(const mxflib::UL &ul) { _sound_compression = ul; }
	  
		virtual const mxflib::UL & getDescriptorUL() const = 0;
		
	  private:
		// SMPTE 377M D.3 Generic sound essence descriptor
		Rational _audio_sampling_rate;
		bool _locked_to_video;
		Int8 _audio_ref_level;
		//Uint8 _electro_spatial_formulation;
		UInt32 _channel_count;
		UInt32 _quantization_bits;
		//Int8 _dial_norm;
		mxflib::UL _sound_compression;
	};


	class WaveAudioDescriptor : public AudioDescriptor
	{
	  public:
		WaveAudioDescriptor(mxflib::MDObjectPtr descriptor);
		WaveAudioDescriptor(Rational sample_rate, Rational audio_sample_rate, UInt32 channel_count, UInt32 quantization_bits);
		WaveAudioDescriptor(const WaveAudioDescriptor &other);
		virtual ~WaveAudioDescriptor() {}
		
		virtual mxflib::MDObjectPtr makeDescriptorObj() const;
		
		virtual UInt8 getGCItemType() const { return 0x16; } // SMPTE 382M-2007 6.5 (i.e. "GC Sound")
		virtual UInt8 getGCElementType() const { return 0x01; } // BWF frame-wrapped
		
		virtual AudioCodec getAudioCodec() const { return AudioCodecUncompressedPCM; }

	  protected:
		virtual const mxflib::UL & getDescriptorUL() const { return mxflib::WaveAudioDescriptor_UL; }
		
	  private:
		// SMPTE 382M-2007 A.1 Wave Audio Essence Descriptor
		UInt16 _block_align;
		UInt32 _avg_bytes_per_sec;
		mxflib::UL _channel_assignment;
		// others...
	};
	

	class AES3Descriptor : public WaveAudioDescriptor
	{
	  public:
		AES3Descriptor(mxflib::MDObjectPtr descriptor);
		AES3Descriptor(Rational sample_rate, Rational audio_sample_rate, UInt32 channel_count, UInt32 quantization_bits);
		AES3Descriptor(const AES3Descriptor &other);
		virtual ~AES3Descriptor() {}
		
		virtual mxflib::MDObjectPtr makeDescriptorObj() const;
		
		virtual UInt8 getGCElementType() const { return 0x03; } // AES3 frame wrapped
		
		virtual AudioCodec getAudioCodec() const { return AudioCodecAES3; }
		
	  protected:
		virtual const mxflib::UL & getDescriptorUL() const { return mxflib::AES3PCMDescriptor_UL; }
		
	  private:
		enum {
			ChannelStatusMode_None = 0,
			ChannelStatusMode_Minimum = 1,
			ChannelStatusMode_Standard = 2,
			ChannelStatusMode_Fixed = 3,
			ChannelStatusMode_Stream = 4,
			ChannelStatusMode_Essence = 5
		};
		
		// SMPTE 382M-2007 A.3 Wave Audio Essence Descriptor
		std::vector<UInt8> _channel_status_mode;
		std::vector< std::vector<unsigned char> > _fixed_channel_status_data;
	};
	
} // namespace

#endif