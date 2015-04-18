/*
 *  Descriptor.cpp
 *  MoxMxf
 *
 *  Created by Brendan Bolles on 4/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxMxf/Descriptor.h>

namespace MoxMxf
{

using namespace mxflib; // so many _UL constants

Descriptor::Descriptor(mxflib::MDObjectPtr descriptor)
{
	_sample_rate = Rational(descriptor->Child(SampleRate_UL)->GetInt("Numerator"), descriptor->Child(SampleRate_UL)->GetInt("Denominator"));
	_container_duration = descriptor->GetInt64(ContainerDuration_UL);
	_essence_container = mxflib::UL(descriptor->Child(EssenceContainer_UL)->GetData().Data);
	
	mxflib::MDObjectPtr codec = descriptor->Child(Codec_UL);
	
	if(codec)
		_codec = mxflib::UL(codec->GetData().Data);
	
	mxflib::MDObjectPtr locators = descriptor->Child(Locators_UL);
	
	assert(!locators); // would mean external essence
}

Descriptor::Descriptor(Rational sample_rate) :
	_sample_rate(sample_rate)
{
	// empty
}

mxflib::MDObjectPtr
Descriptor::makeDescriptorObj() const
{
	mxflib::MDObjectPtr descriptor = new mxflib::MDObject( getDescriptorUL() );
	
	descriptor->AddChild(SampleRate_UL)->SetInt("Numerator", _sample_rate.Numerator);
	descriptor->AddChild(SampleRate_UL)->SetInt("Denominator", _sample_rate.Denominator);
	descriptor->AddChild(ContainerDuration_UL)->SetInt64(_container_duration);
	
	assert(!!_essence_container); // not the default zero value, you have to call setEssenceType()
	descriptor->AddChild(EssenceContainer_UL)->SetValue(_essence_container.GetValue(), _essence_container.Size());
	
	if(!!_codec)
		descriptor->AddChild(Codec_UL)->SetValue(_codec.GetValue(), _codec.Size());
	
	return descriptor;
}

VideoDescriptor::VideoDescriptor(mxflib::MDObjectPtr descriptor) :
	Descriptor(descriptor)
{
	_signal_standard = descriptor->GetUInt(SignalStandard_UL);
	_frame_layout = descriptor->GetUInt(FrameLayout_UL);
	_stored_width = descriptor->GetUInt(StoredWidth_UL);
	_stored_height = descriptor->GetUInt(StoredHeight_UL);
	_stored_f2_offset = descriptor->GetInt(StoredF2Offset_UL);
	_sampled_width = descriptor->GetUInt(SampledWidth_UL);
	_sampled_height = descriptor->GetUInt(SampledHeight_UL);
	_sampled_x_offset = descriptor->GetInt(SampledXOffset_UL);
	_sampled_y_offset = descriptor->GetInt(SampledYOffset_UL);
	_display_width = descriptor->GetUInt(DisplayWidth_UL);
	_display_height = descriptor->GetUInt(DisplayHeight_UL);
	_display_x_offset = descriptor->GetInt(DisplayXOffset_UL);
	_display_y_offset = descriptor->GetInt(DisplayYOffset_UL);
	_display_f2_offset = descriptor->GetInt(DisplayF2Offset_UL);
	_aspect_ratio = Rational(descriptor->Child(AspectRatio_UL)->GetInt("Numerator"), descriptor->Child(AspectRatio_UL)->GetInt("Denominator"));
	_active_format_descriptor = descriptor->GetUInt(ActiveFormatDescriptor_UL);
	
	mxflib::MDObjectPtr video_line_map_array = descriptor->Child(VideoLineMap_UL);
	
	if(video_line_map_array && video_line_map_array->GetClass() == TYPEARRAY)
	{
		for(mxflib::MDObject::const_iterator o_itr = video_line_map_array->begin(); o_itr != video_line_map_array->end(); ++o_itr)
		{
			_video_line_map.push_back( o_itr->second->GetInt() );
		}
	}
	
	_alpha_transparency = descriptor->GetUInt(AlphaTransparency_UL);
	_capture_gamma = mxflib::UL(descriptor->Child(TransferCharacteristic_UL)->GetData().Data);
	_image_alignment_offset = descriptor->GetUInt(ImageAlignmentOffset_UL);
	_image_start_offset = descriptor->GetUInt(ImageStartOffset_UL);
	_image_end_offset = descriptor->GetUInt(ImageEndOffset_UL);
	_field_dominance = descriptor->GetUInt(FieldDominance_UL);
	_picture_essence_coding = UL(descriptor->Child(PictureEssenceCoding_UL)->GetData().Data);
}

// Capture Gamma
static const UInt8 Gamma_BT470_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00 };
const mxflib::UL Gamma_BT470_UL(Gamma_BT470_Data);

static const UInt8 Gamma_BT709_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x02, 0x00, 0x00 };
static const mxflib::UL Gamma_BT709_UL(Gamma_BT709_Data);

static const UInt8 Gamma_SMPTE240M_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x00 };
static const mxflib::UL Gamma_SMPTE240M_UL(Gamma_SMPTE240M_Data);

static const UInt8 Gamma_SMPTE274_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x04, 0x00, 0x00 };
static const mxflib::UL Gamma_SMPTE274_UL(Gamma_SMPTE274_Data);

static const UInt8 Gamma_BT1361_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x05, 0x00, 0x00 };
static const mxflib::UL Gamma_BT1361_UL(Gamma_BT1361_Data);

static const UInt8 Gamma_Linear_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x06, 0x00, 0x00 };
static const mxflib::UL Gamma_Linear_UL(Gamma_Linear_Data);

static const UInt8 Gamma_DC28_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x07, 0x00, 0x00 };
static const mxflib::UL Gamma_DC28_UL(Gamma_DC28_Data);
		
VideoDescriptor::VideoDescriptor(Rational sample_rate, UInt32 width, UInt32 height) :
	Descriptor(sample_rate),
	_signal_standard(0),
	_frame_layout(LayoutType_FullFrame),
	_stored_width(width),
	_stored_height(height),
	_stored_f2_offset(0),
	_sampled_width(width),
	_sampled_height(height),
	_sampled_x_offset(0),
	_sampled_y_offset(0),
	_display_width(width),
	_display_height(height),
	_display_x_offset(0),
	_display_y_offset(0),
	_display_f2_offset(0),
	//_aspect_ratio(Rational(width, height)),
	_active_format_descriptor(0),
	_alpha_transparency(0),
	_capture_gamma(Gamma_BT709_UL),
	_image_alignment_offset(0),
	_image_start_offset(0),
	_image_end_offset(0),
	_field_dominance(1)
{
	// sure you don't want to adopt mxflib's Rational?
	mxflib::Rational aspect(width, height);
	aspect.Reduce();
	_aspect_ratio = Rational(aspect.Numerator, aspect.Denominator);

	_video_line_map.resize(2);
	_video_line_map[0] = _video_line_map[1] = 0;
}

mxflib::MDObjectPtr
VideoDescriptor::makeDescriptorObj() const
{
	mxflib::MDObjectPtr descriptor = Descriptor::makeDescriptorObj();
	
	descriptor->AddChild(SignalStandard_UL)->SetUInt(_signal_standard);
	descriptor->AddChild(FrameLayout_UL)->SetUInt(_frame_layout);
	descriptor->AddChild(StoredWidth_UL)->SetUInt(_stored_width);
	descriptor->AddChild(StoredHeight_UL)->SetUInt(_stored_height);
	descriptor->AddChild(StoredF2Offset_UL)->SetInt(_stored_f2_offset);
	descriptor->AddChild(SampledWidth_UL)->SetUInt(_sampled_width);
	descriptor->AddChild(SampledHeight_UL)->SetUInt(_sampled_height);
	descriptor->AddChild(SampledXOffset_UL)->SetInt(_sampled_x_offset);
	descriptor->AddChild(SampledYOffset_UL)->SetInt(_sampled_y_offset);
	descriptor->AddChild(DisplayWidth_UL)->SetUInt(_display_width);
	descriptor->AddChild(DisplayHeight_UL)->SetUInt(_display_height);
	descriptor->AddChild(DisplayXOffset_UL)->SetInt(_display_x_offset);
	descriptor->AddChild(DisplayYOffset_UL)->SetInt(_display_y_offset);
	descriptor->AddChild(DisplayF2Offset_UL)->SetInt(_display_f2_offset);
	descriptor->AddChild(AspectRatio_UL)->SetInt("Numerator", _aspect_ratio.Numerator);
	descriptor->AddChild(AspectRatio_UL)->SetInt("Denominator", _aspect_ratio.Denominator);
	descriptor->AddChild(ActiveFormatDescriptor_UL)->SetUInt(_active_format_descriptor);
	
	if(_video_line_map.size() > 0)
	{
		mxflib::MDObjectPtr video_line_map_array = descriptor->AddChild(VideoLineMap_UL);
		
		for(int i=0; i < _video_line_map.size(); i++)
		{
			video_line_map_array->AddChild()->SetInt( _video_line_map[i] );
		}
		
		assert(video_line_map_array->GetClass() == TYPEARRAY);
	}
	
	descriptor->AddChild(AlphaTransparency_UL)->SetUInt(_alpha_transparency);
	descriptor->AddChild(TransferCharacteristic_UL)->SetValue(_capture_gamma.GetValue(), _capture_gamma.Size());
	descriptor->AddChild(ImageAlignmentOffset_UL)->SetUInt(_image_alignment_offset);
	descriptor->AddChild(ImageStartOffset_UL)->SetUInt(_image_start_offset);
	descriptor->AddChild(ImageEndOffset_UL)->SetUInt(_image_end_offset);
	descriptor->AddChild(FieldDominance_UL)->SetUInt(_field_dominance);
	
	assert(!!_picture_essence_coding); // you must call setPictureEssenceCoding()
	descriptor->AddChild(PictureEssenceCoding_UL)->SetValue(_picture_essence_coding.GetValue(), _picture_essence_coding.Size());
	
	return descriptor;
}

CDCIDescriptor::CDCIDescriptor(mxflib::MDObjectPtr descriptor) :
	VideoDescriptor(descriptor)
{
	_component_depth = descriptor->GetUInt(ComponentDepth_UL);
	_horizontal_subsampling = descriptor->GetUInt(HorizontalSubsampling_UL);
	_vertical_subsampling = descriptor->GetUInt(VerticalSubsampling_UL);
	_color_siting = descriptor->GetUInt(ColorSiting_UL);
	_reversed_byte_order = descriptor->GetUInt(ReversedByteOrder_UL);
	_padding_bits = descriptor->GetInt(PaddingBits_UL);
	_alpha_sample_depth = descriptor->GetUInt(AlphaSampleDepth_UL);
	_black_ref_level = descriptor->GetUInt(BlackRefLevel_UL);
	_white_ref_level = descriptor->GetUInt(WhiteReflevel_UL);
	_color_range = descriptor->GetUInt(ColorRange_UL);
}

CDCIDescriptor::CDCIDescriptor(Rational sample_rate, UInt32 width, UInt32 height, UInt32 horizontal_subsampling, UInt32 vertical_subsampling) :
	VideoDescriptor(sample_rate, width, height),
	_component_depth(8),
	_horizontal_subsampling(horizontal_subsampling),
	_vertical_subsampling(vertical_subsampling),
	_color_siting(ColorSiting_CoSiting),
	_reversed_byte_order(false),
	_padding_bits(0),
	_alpha_sample_depth(0),
	_black_ref_level(0),
	_white_ref_level(255),
	_color_range(255)
{
	// this should get over-written if MPEG descriptor used
	setEssenceType(MXFGCUncompressed_UL);
	
	static const UInt8 GC_Uncompressed_FrameWrapped_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x00, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x05, 0x7f, 0x01 };
	static const mxflib::UL GC_Uncompressed_FrameWrapped_UL(GC_Uncompressed_FrameWrapped_Data);

	setPictureEssenceCoding(GC_Uncompressed_FrameWrapped_UL);
}

mxflib::MDObjectPtr
CDCIDescriptor::makeDescriptorObj() const
{
	mxflib::MDObjectPtr descriptor = VideoDescriptor::makeDescriptorObj();
	
	descriptor->AddChild(ComponentDepth_UL)->SetUInt(_component_depth);
	descriptor->AddChild(HorizontalSubsampling_UL)->SetUInt(_horizontal_subsampling);
	descriptor->AddChild(VerticalSubsampling_UL)->SetUInt(_vertical_subsampling);
	descriptor->AddChild(ColorSiting_UL)->SetUInt(_color_siting);
	descriptor->AddChild(ReversedByteOrder_UL)->SetUInt(_reversed_byte_order);
	descriptor->AddChild(PaddingBits_UL)->SetInt(_padding_bits);
	descriptor->AddChild(AlphaSampleDepth_UL)->SetUInt(_alpha_sample_depth);
	descriptor->AddChild(BlackRefLevel_UL)->SetUInt(_black_ref_level);
	descriptor->AddChild(WhiteReflevel_UL)->SetUInt(_white_ref_level);
	descriptor->AddChild(ColorRange_UL)->SetUInt(_color_range);
	
	return descriptor;
}

RGBADescriptor::RGBADescriptor(mxflib::MDObjectPtr descriptor) :
	VideoDescriptor(descriptor)
{
	_component_max_ref = descriptor->GetUInt(ComponentMaxRef_UL);
	_component_min_ref = descriptor->GetUInt(ComponentMinRef_UL);
	_alpha_max_ref = descriptor->GetUInt(AlphaMaxRef_UL);
	_alpha_min_ref = descriptor->GetUInt(AlphaMinRef_UL);
	_scanning_direction = descriptor->GetUInt(ScanningDirection_UL);
	
	mxflib::MDObjectPtr pixel_layout_array = descriptor->Child(PixelLayout_UL);
	
	if(pixel_layout_array && pixel_layout_array->GetClass() == mxflib::TYPEARRAY)
	{
		assert(*pixel_layout_array->GetValueType()->GetTypeUL() == RGBALayout_UL); // not sure about this
	
		for(mxflib::MDObject::const_iterator o_itr = pixel_layout_array->begin(); o_itr != pixel_layout_array->end(); ++o_itr)
		{
			assert(o_itr->second->GetClass() == mxflib::ENUM);
			assert(*o_itr->second->GetValueType()->GetTypeUL() == RGBALayoutItem_UL); // not sure about this
		
			_pixel_layout.push_back( o_itr->second->GetUInt() );
		}
	}
}

RGBADescriptor::RGBADescriptor(Rational sample_rate, UInt32 width, UInt32 height) :
	VideoDescriptor(sample_rate, width, height),
	_component_max_ref(255),
	_component_min_ref(0),
	_alpha_max_ref(255),
	_alpha_min_ref(0),
	_scanning_direction(ScanningDir_LRTB)
{
	assert(false); // don't know what I'm doing!

	// see SMPTE 377M E.2.46 (page 99)
	_pixel_layout.push_back('R');
	_pixel_layout.push_back(8);
	_pixel_layout.push_back('G');
	_pixel_layout.push_back(8);
	_pixel_layout.push_back('B');
	_pixel_layout.push_back(8);
	_pixel_layout.push_back(0);
	_pixel_layout.push_back(0);
	
	setEssenceType(MXFGCUncompressed_UL);
	
	static const UInt8 GC_Uncompressed_FrameWrapped_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x00, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x05, 0x7f, 0x01 };
	static const mxflib::UL GC_Uncompressed_FrameWrapped_UL(GC_Uncompressed_FrameWrapped_Data);

	setPictureEssenceCoding(GC_Uncompressed_FrameWrapped_UL);
}

mxflib::MDObjectPtr
RGBADescriptor::makeDescriptorObj() const
{
	mxflib::MDObjectPtr descriptor = VideoDescriptor::makeDescriptorObj();
	
	descriptor->AddChild(ComponentMaxRef_UL)->SetUInt(_component_max_ref);
	descriptor->AddChild(ComponentMinRef_UL)->SetUInt(_component_min_ref);
	descriptor->AddChild(AlphaMaxRef_UL)->SetUInt(_alpha_max_ref);
	descriptor->AddChild(AlphaMinRef_UL)->SetUInt(_alpha_min_ref);
	descriptor->AddChild(ScanningDirection_UL)->SetUInt(_scanning_direction);

	if(_pixel_layout.size() > 0)
	{
		mxflib::MDObjectPtr pixel_layout_array = descriptor->AddChild(PixelLayout_UL);
		
		for(int i=0; i < _pixel_layout.size(); i++)
		{
			pixel_layout_array->AddChild()->SetUInt( _pixel_layout[i] );
		}
		
		assert(pixel_layout_array->GetClass() == mxflib::TYPEARRAY);
	}
	
	return descriptor;
}


MPEGDescriptor::MPEGDescriptor(mxflib::MDObjectPtr descriptor) :
	CDCIDescriptor(descriptor)
{
	_single_sequence = descriptor->GetUInt(SingleSequence_UL);
	_constant_b_picture = descriptor->GetUInt(ConstantBFrames_UL);
	_coded_content_type = descriptor->GetUInt(CodedContentType_UL);
	_low_delay = descriptor->GetUInt(LowDelay_UL);
	_closed_GOP = descriptor->GetUInt(ClosedGOP_UL);
	_identical_GOP = descriptor->GetUInt(IdenticalGOP_UL);
	_maximum_GOP_size = descriptor->GetUInt(MaxGOP_UL);
	_maximum_b_picture_count = descriptor->GetUInt(BPictureCount_UL);
	_bit_rate = descriptor->GetUInt(BitRate_UL);
	_profile_and_level = descriptor->GetUInt(ProfileAndLevel_UL);
}

MPEGDescriptor::MPEGDescriptor(Rational sample_rate, UInt32 width, UInt32 height, UInt32 horizontal_subsampling, UInt32 vertical_subsampling) :
	CDCIDescriptor(sample_rate, width, height, horizontal_subsampling, vertical_subsampling),
	_single_sequence(false),
	_constant_b_picture(false),
	_coded_content_type(CodedType_Unknown),
	_low_delay(false),
	_closed_GOP(false),
	_identical_GOP(false),
	_maximum_GOP_size(0),
	_maximum_b_picture_count(2),
	_bit_rate(0),
	_profile_and_level(82)
{
	assert(false); // This is not ready for prime time!  Just using values from my sample file.
	
	setEssenceType(MXFGCMPEGES_UL);
	
	// picked this value out of dozens of options in RP224
	static const UInt8 MPEG2_422P_HL_LongGOP_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x03, 0x04, 0x01, 0x02, 0x02, 0x01, 0x04, 0x03, 0x00 };
	static const mxflib::UL MPEG2_422P_HL_LongGOP_UL(MPEG2_422P_HL_LongGOP_Data);

	setPictureEssenceCoding(MPEG2_422P_HL_LongGOP_UL);
}


mxflib::MDObjectPtr
MPEGDescriptor::makeDescriptorObj() const
{
	mxflib::MDObjectPtr descriptor = CDCIDescriptor::makeDescriptorObj();
	
	descriptor->AddChild(SingleSequence_UL)->SetUInt(_single_sequence);
	descriptor->AddChild(ConstantBFrames_UL)->SetUInt(_constant_b_picture);
	descriptor->AddChild(CodedContentType_UL)->SetUInt(_coded_content_type);
	descriptor->AddChild(LowDelay_UL)->SetUInt(_low_delay);
	descriptor->AddChild(ClosedGOP_UL)->SetUInt(_closed_GOP);
	descriptor->AddChild(IdenticalGOP_UL)->SetUInt(_identical_GOP);
	descriptor->AddChild(MaxGOP_UL)->SetUInt(_maximum_GOP_size);
	descriptor->AddChild(BPictureCount_UL)->SetUInt(_maximum_b_picture_count);
	descriptor->AddChild(BitRate_UL)->SetUInt(_bit_rate);
	descriptor->AddChild(ProfileAndLevel_UL)->SetUInt(_profile_and_level);
	
	return descriptor;
}


AudioDescriptor::AudioDescriptor(mxflib::MDObjectPtr descriptor) :
	Descriptor(descriptor)
{
	_audio_sample_rate = Rational(descriptor->Child(AudioSamplingRate_UL)->GetInt("Numerator"), descriptor->Child(AudioSamplingRate_UL)->GetInt("Denominator"));
	_locked_to_video = descriptor->GetUInt(Locked_UL);
	_audio_ref_level = descriptor->GetUInt(AudioRefLevel_UL);
	_channel_count = descriptor->GetUInt(ChannelCount_UL);
	_quantization_bits = descriptor->GetUInt(QuantizationBits_UL);
	
	mxflib::MDObjectPtr sound_compression = descriptor->Child(SoundEssenceCompression_UL);
	
	if(sound_compression)
		_sound_compression = mxflib::UL(sound_compression->GetData().Data);
}

AudioDescriptor::AudioDescriptor(Rational sample_rate, Rational audio_sample_rate, UInt32 channel_count, UInt32 quantization_bits) :
	Descriptor(sample_rate),
	_audio_sample_rate(audio_sample_rate),
	_locked_to_video(true),
	_audio_ref_level(0),
	//_electro_spatial_formulation(),
	_channel_count(channel_count),
	_quantization_bits(quantization_bits)
	//_dial_norm()
{
	// empty
}

mxflib::MDObjectPtr
AudioDescriptor::makeDescriptorObj() const
{
	mxflib::MDObjectPtr descriptor = Descriptor::makeDescriptorObj();

	descriptor->AddChild(AudioSamplingRate_UL)->SetInt("Numerator", _audio_sample_rate.Numerator);
	descriptor->AddChild(AudioSamplingRate_UL)->SetInt("Denominator", _audio_sample_rate.Denominator);
	descriptor->AddChild(Locked_UL)->SetUInt(_locked_to_video);
	descriptor->AddChild(AudioRefLevel_UL)->SetUInt(_audio_ref_level);
	descriptor->AddChild(ChannelCount_UL)->SetUInt(_channel_count);
	descriptor->AddChild(QuantizationBits_UL)->SetUInt(_quantization_bits);
	
	if(!!_sound_compression)
		descriptor->AddChild(SoundEssenceCompression_UL)->SetValue(_sound_compression.GetValue(), _sound_compression.Size());

	return descriptor;
}

WaveAudioDescriptor::WaveAudioDescriptor(mxflib::MDObjectPtr descriptor) :
	AudioDescriptor(descriptor)
{
	_block_align = descriptor->GetUInt(BlockAlign_UL);
	_avg_bytes_per_sec = descriptor->GetUInt(AvgBps_UL);
}

WaveAudioDescriptor::WaveAudioDescriptor(Rational sample_rate, Rational audio_sample_rate, UInt32 channel_count, UInt32 quantization_bits) :
	AudioDescriptor(sample_rate, audio_sample_rate, channel_count, quantization_bits),
	_block_align(3),
	_avg_bytes_per_sec(144000)
{
	assert(false); // not ready yet
}


mxflib::MDObjectPtr
WaveAudioDescriptor::makeDescriptorObj() const
{
	mxflib::MDObjectPtr descriptor = AudioDescriptor::makeDescriptorObj();

	descriptor->AddChild(BlockAlign_UL)->SetUInt(_block_align);
	descriptor->AddChild(AvgBps_UL)->SetUInt(_avg_bytes_per_sec);

	return descriptor;
}

AES3Descriptor::AES3Descriptor(mxflib::MDObjectPtr descriptor) :
	WaveAudioDescriptor(descriptor)
{
	mxflib::MDObjectPtr channel_status_mode_array = descriptor->Child(ChannelStatusMode_UL);
	
	if(channel_status_mode_array && channel_status_mode_array->GetClass() == mxflib::TYPEARRAY)
	{
		for(mxflib::MDObject::const_iterator o_itr = channel_status_mode_array->begin(); o_itr != channel_status_mode_array->end(); ++o_itr)
		{
			assert(o_itr->second->GetClass() == mxflib::ENUM);
			assert(*o_itr->second->GetValueType()->GetTypeUL() == ChannelStatusModeType_UL);
		
			_channel_status_mode.push_back( o_itr->second->GetUInt() );
		}
	}
	
	
	mxflib::MDObjectPtr fixed_channel_status_data_array = descriptor->Child(FixedChannelStatusData_UL);
	
	if(fixed_channel_status_data_array && fixed_channel_status_data_array->GetClass() == mxflib::TYPEARRAY)
	{
		assert(fixed_channel_status_data_array->GetArrayClass() == mxflib::ARRAYEXPLICIT);
	
		for(mxflib::MDObject::const_iterator o_itr = fixed_channel_status_data_array->begin(); o_itr != fixed_channel_status_data_array->end(); ++o_itr)
		{
			assert(o_itr->second->GetClass() == mxflib::TYPEARRAY);
			assert(o_itr->second->GetArrayClass() == mxflib::ARRAYIMPLICIT);
		
			const mxflib::DataChunk &dat = o_itr->second->GetData();
			
			std::vector<unsigned char> status_data(dat.Size);
			
			for(int i=0; i < dat.Size; i++)
			{
				status_data[i] = dat.Data[i];
			}
		
			_fixed_channel_status_data.push_back( status_data );
		}
	}
}

AES3Descriptor::AES3Descriptor(Rational sample_rate, Rational audio_sample_rate, UInt32 channel_count, UInt32 quantization_bits) :
	WaveAudioDescriptor(sample_rate, audio_sample_rate, channel_count, quantization_bits)
{
	assert(false); // not doing anything right here yet
	
	static const UInt8 AES3_Sound_Coding_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0a, 0x04, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00 };
	static const mxflib::UL AES3_Sound_Coding_UL(AES3_Sound_Coding_Data);
	
	setSoundCompression(AES3_Sound_Coding_UL);
}

mxflib::MDObjectPtr
AES3Descriptor::makeDescriptorObj() const
{
	mxflib::MDObjectPtr descriptor = WaveAudioDescriptor::makeDescriptorObj();

	if(_channel_status_mode.size() > 0)
	{
		mxflib::MDObjectPtr channel_status_mode_array = descriptor->AddChild(ChannelStatusMode_UL);
		
		for(int i=0; i < _channel_status_mode.size(); i++)
		{
			channel_status_mode_array->AddChild()->SetUInt( _channel_status_mode[i] );
		}
		
		assert(channel_status_mode_array->GetClass() == mxflib::TYPEARRAY);
	}
	
	//if(_fixed_channel_status_data.size() > 0)
	//{
	//	MDObjectPtr fixed_channel_status_data_array = descriptor->AddChild(FixedChannelStatusData_UL);
	//	
	//	for(int i=0; i < _fixed_channel_status_data.size(); i++)
	//	{
	//		
	//	}
	//}

	return descriptor;
}

} // namespace