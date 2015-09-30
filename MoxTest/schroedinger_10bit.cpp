/*
 *  schroedinger_10bit.cpp
 *  MOX_AE
 *
 *  Created by Brendan Bolles on 9/29/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */


#include <iostream>

#include <assert.h>

#include <schroedinger/schro.h>


template <typename T>
static void
FillFrame(SchroFrame *frame, int luma_min, int luma_max, int chroma_zero)
{
	for(int y = 0; y < frame->height; y++)
	{
		T *Y = (T *)((char *)frame->components[0].data + (y * frame->components[0].stride));
		T *Cb = (T *)((char *)frame->components[1].data + (y * frame->components[1].stride));
		T *Cr = (T *)((char *)frame->components[2].data + (y * frame->components[2].stride));
		
		for(int x = 0; x < frame->width; x++)
		{
			*Y++ = (float)luma_min + ((float)(luma_max - luma_min) * (float)x / (float)(frame->width - 1)) + 0.5f;
			*Cb++ = chroma_zero;
			*Cr++ = chroma_zero;
		}
	}
}


template <typename T>
static void
PrintFirstLine(SchroFrame *frame)
{
	// luma only
	T *Y = (T *)frame->components[0].data;
	
	// first line only
	for(int x = 0; x < frame->width; x++)
	{
		std::cout << (int)*Y++ << " ";
	}
	
	std::cout << std::endl;
}


static void
PrintFirstLine(SchroFrame *frame)
{
	if(frame->components[0].format == SCHRO_FRAME_FORMAT_U8_444)
		PrintFirstLine<unsigned char>(frame);
	else if(frame->components[0].format == SCHRO_FRAME_FORMAT_S16_444)
		PrintFirstLine<short>(frame);
}


template <typename T>
static bool
CompareFrames(SchroFrame *frame1, SchroFrame *frame2)
{
	for(int y = 0; y < frame1->height; y++)
	{
		T *Y1 = (T *)((char *)frame1->components[0].data + (y * frame1->components[0].stride));
		T *Y2 = (T *)((char *)frame2->components[0].data + (y * frame2->components[0].stride));
		
		for(int x = 0; x < frame1->width; x++)
		{
			if(*Y1++ != *Y2++)
				return false;
		}
	}
	
	return true;
}

static bool
CompareFrames(SchroFrame *frame1, SchroFrame *frame2)
{
	if(frame1->components[0].format != frame2->components[0].format ||
		frame1->width != frame2->width ||
		frame1->height != frame2->height)
		return false;
	
	if(frame1->components[0].format == SCHRO_FRAME_FORMAT_U8_444)
		return CompareFrames<unsigned char>(frame1, frame2);
	else if(frame1->components[0].format == SCHRO_FRAME_FORMAT_S16_444)
		return CompareFrames<short>(frame1, frame2);
	else
		return false;
}


static bool
RunTest(int bit_depth)
{
	bool result = true;

	const int seq_len = 5;
	
	const int width = 100;
	const int height = 10;
	
	int luma_min = 16;
	int luma_max = 235;
	int chroma_zero = 128;
	
	schro_init();
	
	// set up encoder
	SchroEncoder *encoder = schro_encoder_new();
	
	schro_encoder_setting_set_double(encoder, "gop_structure", SCHRO_ENCODER_GOP_INTRA_ONLY);
	schro_encoder_setting_set_double(encoder, "rate_control", SCHRO_ENCODER_RATE_CONTROL_LOSSLESS);
	//schro_encoder_setting_set_double(encoder, "force_profile", SCHRO_ENCODER_PROFILE_VC2_SIMPLE);
	//schro_encoder_setting_set_double(encoder, "queue_depth", seq_len);
	//assert(seq_len <= SCHRO_LIMIT_FRAME_QUEUE_LENGTH);
	
	SchroVideoFormat *format = schro_encoder_get_video_format(encoder);
	
	if(format)
	{
		format->width = width;
		format->height = height;
		
		format->clean_width = format->width;
		format->clean_height = format->height;
		format->left_offset = 0;
		format->top_offset = 0;
		
		format->chroma_format = SCHRO_CHROMA_444;
		
		const SchroSignalRange range = (bit_depth == 12 ? SCHRO_SIGNAL_RANGE_12BIT_VIDEO :
										bit_depth == 10 ? SCHRO_SIGNAL_RANGE_10BIT_VIDEO :
										SCHRO_SIGNAL_RANGE_8BIT_VIDEO);
										
		schro_video_format_set_std_signal_range(format, range);
		
		luma_min = format->luma_offset;
		luma_max = format->luma_offset + format->luma_excursion;
		chroma_zero = format->chroma_offset;
		
		format->colour_primaries = SCHRO_COLOUR_PRIMARY_HDTV;
		format->colour_matrix = SCHRO_COLOUR_MATRIX_HDTV;
		format->transfer_function = SCHRO_TRANSFER_CHAR_TV_GAMMA;
		
		format->interlaced = false;
		
		format->frame_rate_numerator = 24;
		format->frame_rate_denominator = 1;
		
		format->aspect_ratio_numerator = 1;
		format->aspect_ratio_denominator = 1;
		
		schro_encoder_set_video_format(encoder, format);
		
		free(format);
	}
	else
		return false;
	
	schro_encoder_start(encoder);
	
	
	// create frame
	SchroFrame *start_frame = schro_frame_new_and_alloc(NULL, SCHRO_FRAME_FORMAT_U8_444, width, height);
	
	FillFrame<unsigned char>(start_frame, 16, 235, 128);
	
	const SchroFrameFormat schro_format = (bit_depth > 8 ? SCHRO_FRAME_FORMAT_S16_444 : SCHRO_FRAME_FORMAT_U8_444);

	SchroFrame *original_frame = schro_frame_new_and_alloc(NULL, schro_format, width, height);
	
	schro_frame_convert(original_frame, start_frame);
	
	
	
	SchroDecoder *decoder = schro_decoder_new();
	
	// push frames to encoder
	for(int t = 0; t < seq_len; t++)
	{
		SchroFrame *new_frame = schro_frame_dup(original_frame);
	
		schro_encoder_push_frame(encoder, new_frame);
	}
	
	
	
	// pull packets out of encoder, pass to decoder
	int packets_out = 0;
	
	while(packets_out < seq_len)
	{
		SchroStateEnum encoder_state = schro_encoder_wait(encoder);
		
		if(encoder_state == SCHRO_STATE_HAVE_BUFFER || encoder_state == SCHRO_STATE_END_OF_STREAM)
		{
			int n_decodable_frames = -1;
		
			SchroBuffer *buffer = schro_encoder_pull(encoder, &n_decodable_frames);
			
			if(buffer)
			{
				const int parse_code = buffer->data[4];
				
				if(SCHRO_PARSE_CODE_IS_SEQ_HEADER(parse_code) ||
					SCHRO_PARSE_CODE_IS_AUXILIARY_DATA(parse_code) ||
					SCHRO_PARSE_CODE_IS_PICTURE(parse_code))
				{
					schro_decoder_push(decoder, buffer);
					
					//schro_buffer_unref(buffer);
					
					if(SCHRO_PARSE_CODE_IS_PICTURE(parse_code))
					{
						packets_out++;
					}
				}
			}
		}
		else
		{
			assert(encoder_state == SCHRO_STATE_NEED_FRAME);
			assert(encoder_state != SCHRO_STATE_AGAIN); // yeah, redundant
		
			schro_encoder_end_of_stream(encoder);
		}
	}
	
	
	
	// pull frames out of decoder
	int frames_out = 0;
	
	while(frames_out < seq_len)
	{
		int decoder_state = schro_decoder_wait(decoder);
		
		if(decoder_state == SCHRO_DECODER_FIRST_ACCESS_UNIT)
		{
			SchroVideoFormat *format = schro_decoder_get_video_format(decoder);
			
			if(format)
			{
				assert(format->width == width);
				assert(format->height == height);
				
				assert(format->chroma_format == SCHRO_CHROMA_444);
				
				assert(format->luma_offset == luma_min);
				assert(format->luma_offset + format->luma_excursion == luma_max);
				assert(format->chroma_offset = chroma_zero);
				
				free(format);
			}
		}
		else if(decoder_state == SCHRO_DECODER_NEED_BITS)
		{
			schro_decoder_push_end_of_stream(decoder);
		}
		else if(decoder_state == SCHRO_DECODER_NEED_FRAME)
		{
			SchroFrame *decoder_frame = schro_frame_new_and_alloc(NULL, schro_format, width, height);
			
			schro_decoder_add_output_picture(decoder, decoder_frame);
		}
		else if(decoder_state == SCHRO_DECODER_OK || decoder_state == SCHRO_DECODER_EOS)
		{
			SchroFrame *decoder_frame = schro_decoder_pull(decoder);
			
			if(decoder_frame)
			{
				frames_out++;
			
				bool match = CompareFrames(decoder_frame, original_frame);
				
				//std::cout << (match ? "Match!" : "No Match!") << "  " << std::endl;
				
				if(!match)
				{
					// output doesn't match input, so print the values of the
					// first line of the first component to see what went in and out
					PrintFirstLine(original_frame);
					std::cout << "==========" << std::endl;
					PrintFirstLine(decoder_frame);
					std::cout << "==========" << std::endl;
				
					result = false;
				}
				
				schro_frame_unref(decoder_frame);
			}
		}
	}

	schro_frame_unref(original_frame);
	schro_frame_unref(start_frame);
	
	schro_decoder_free(decoder);
	schro_encoder_free(encoder);
	
	return result;
}


int main(int argc, char * const argv[])
{
	std::cout << "8-bit test...";
	const bool test8 = RunTest(8);
	std::cout << (test8 ? "successful" : "FAILED") << std::endl;

	std::cout << "10-bit test...";
	const bool test10 = RunTest(10);
	std::cout << (test10 ? "successful" : "FAILED") << std::endl;

	std::cout << "12-bit test...";
	const bool test12 = RunTest(12);
	std::cout << (test12 ? "successful" : "FAILED") << std::endl;

	
	const bool success = (test8 && test10 && test12);

    return (success ? 0 : -1);
}