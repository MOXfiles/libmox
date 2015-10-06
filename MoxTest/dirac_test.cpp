/*
 *  dirac_test.cpp
 *  MOX_AE
 *
 *  Created by Brendan Bolles on 10/5/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */


/*
	The point of this file is to see how the dirac-research encoder might help us encode/decode 10-bit and higher.
	
	The good news is that dirac-research does do this.  The bad news is that it's 1) very slow and 2) unable to pick up a stream
	mid-way the like Schroedinger can.  Even if I append the sequence header to every frame it doesn't seem to work.  Unless that
	is fixed, it can't be used in MOX, which requires random access to the stream.
	
	Perhaps in the future this test could be used to fix Schroedinger for >8 bits.  We can write 10-bit packets with
	dirac-research, we just have to figure out where Schroedinger messes up while reading them.
*/


#include <iostream>

#include <queue>

#include <assert.h>

#include <libdirac_encoder/dirac_encoder.h>
#include <libdirac_decoder/dirac_parser.h>

#include <libdirac_decoder/dirac_cppparser.h>
#include <libdirac_common/picture.h>

#include <schroedinger/schro.h>


static const int width = 100;
static const int height = 50;

static const int sequence_len = 15;


typedef std::queue<dirac_enc_data_t> packets;


template <typename T>
static void
PrintLine(T *buf)
{
	std::cout << "===Line=====" << std::endl;

	for(int x = 0; x < width; x++)
	{
		std::cout << (int)*buf++ << " ";
	}
	
	std::cout << std::endl;
	
	std::cout << "============" << std::endl;
}


static void
PrintLine(const dirac::PicArray &comp)
{
	std::cout << "===Line=====" << std::endl;
	
	for(int x = 0; x < comp.LengthX(); x++)
	{
		std::cout << (int)comp[0][x] << " ";
	}
	
	std::cout << std::endl;
	
	std::cout << "============" << std::endl;
}


template <typename T>
static dirac_enc_data_t
CreateFrame(int luma_min, int luma_delta, int chroma_zero)
{
	dirac_enc_data_t enc_data;

	const size_t subpixel_size = sizeof(T);
	const size_t rowbytes = subpixel_size * width;
	const size_t channel_size = rowbytes * height;
	const size_t data_size = channel_size * 3;
	
	enc_data.size = data_size;
	enc_data.buffer = (unsigned char *)malloc(enc_data.size);
	
	for(int c = 0; c < 3; c++)
	{
		T *chan = (T *)((unsigned char *)enc_data.buffer + (c * channel_size));
	
		for(int y = 0; y < height; y++)
		{
			T *pix = (T *)((unsigned char *)chan + (y * rowbytes));
			
			for(int x = 0; x < width; x++)
			{
				if(c == 0)
				{
					*pix++ = (float)luma_min + ((float)luma_delta * (float)x / (float)(width - 1)) + 0.5f;
				}
				else
					*pix++ = chroma_zero;
			}
		}
	}
	
	return enc_data;
}

template <typename BUFTYPE, int BITDEPTH>
static packets
compress_video(bool tack_on_header = false)
{
	packets out;

	// create encoder
	dirac_encoder_context_t encoder_ctx;

	dirac_encoder_context_init(&encoder_ctx, VIDEO_FORMAT_CUSTOM);
	
	encoder_ctx.src_params.width = width;
	encoder_ctx.src_params.height = height;
	encoder_ctx.src_params.chroma = format444;
	encoder_ctx.src_params.chroma_width = encoder_ctx.src_params.width;
	encoder_ctx.src_params.chroma_height = encoder_ctx.src_params.height;
	encoder_ctx.src_params.source_sampling = 0; // progressive
	encoder_ctx.src_params.topfieldfirst = 0;
	encoder_ctx.src_params.frame_rate.numerator = 24;
	encoder_ctx.src_params.frame_rate.denominator = 1;
	encoder_ctx.src_params.pix_asr.numerator = 1;
	encoder_ctx.src_params.pix_asr.denominator = 1;
	encoder_ctx.src_params.clean_area.width = encoder_ctx.src_params.width;
	encoder_ctx.src_params.clean_area.height = encoder_ctx.src_params.height;
	encoder_ctx.src_params.clean_area.left_offset = 0;
	encoder_ctx.src_params.clean_area.top_offset = 0;
	
	if(BITDEPTH == 8)
	{
		encoder_ctx.src_params.signal_range.luma_offset = 16;
		encoder_ctx.src_params.signal_range.luma_excursion = 219;
		encoder_ctx.src_params.signal_range.chroma_offset = 128;
		encoder_ctx.src_params.signal_range.chroma_excursion = 224;
	}
	else if(BITDEPTH == 10)
	{
		encoder_ctx.src_params.signal_range.luma_offset = 64;
		encoder_ctx.src_params.signal_range.luma_excursion = 876;
		encoder_ctx.src_params.signal_range.chroma_offset = 512;
		encoder_ctx.src_params.signal_range.chroma_excursion = 896;
	}
	else if(BITDEPTH == 12)
	{
		encoder_ctx.src_params.signal_range.luma_offset = 256;
		encoder_ctx.src_params.signal_range.luma_excursion = 3504;
		encoder_ctx.src_params.signal_range.chroma_offset = 2048;
		encoder_ctx.src_params.signal_range.chroma_excursion = 3584;
	}
	
	encoder_ctx.src_params.colour_spec.col_primary = CP_SDTV_525;
	encoder_ctx.src_params.colour_spec.col_matrix.kr = 0.299f; // see libdirac_common/dirac_parser.cpp
	encoder_ctx.src_params.colour_spec.col_matrix.kb = 0.114f;
	encoder_ctx.src_params.colour_spec.trans_func = TF_TV;
	
	
	encoder_ctx.enc_params.lossless = true;
	
	encoder_ctx.enc_params.L1_sep = 0;
	encoder_ctx.enc_params.num_L1 = 0; // intra-only
	

	// create frame
	dirac_enc_data_t enc_data = CreateFrame<BUFTYPE>(encoder_ctx.src_params.signal_range.luma_offset,
														encoder_ctx.src_params.signal_range.luma_excursion,
														encoder_ctx.src_params.signal_range.chroma_offset);
	
	BUFTYPE *frame = (BUFTYPE *)enc_data.buffer;
	const size_t data_size = enc_data.size;
	
	PrintLine<BUFTYPE>(frame);
	
	

	dirac_encoder_t *encoder = dirac_encoder_init(&encoder_ctx, true);
	
	if(encoder)
	{
		unsigned char *sequence_header = NULL;
		size_t sequence_header_size = 0;
		
		// make buffer
		unsigned char *video_buf = (unsigned char *)malloc(width * height * sizeof(BUFTYPE) * 3);
		
		assert(video_buf != NULL);
	
		int frames_in = 0;
		int frames_out = 0;
		
		while(frames_out < sequence_len)
		{
			if(frames_in < sequence_len)
			{
				dirac_encoder_load(encoder, (unsigned char *)frame, data_size);
				
				std::cout << "[[Frame " << frames_in << " in]]" << std::endl;
				
				frames_in++;
			}
			else
			{
				dirac_encoder_end_sequence(encoder);
				
				std::cout << "[[EOS signalled]]" << std::endl;
			}
			
			
			bool encoder_pull = true;
			
			while(encoder_pull)
			{
				encoder->enc_buf.buffer = video_buf;
				encoder->enc_buf.size = data_size;
				
				dirac_encoder_state_t state = dirac_encoder_output(encoder);
				
				if(state == ENC_STATE_AVAIL)
				{
					if(encoder->enc_buf.size > 0)
					{
						int buf_size = encoder->enc_buf.size;
					
						unsigned char *buf = encoder->enc_buf.buffer;
					
#define DIRAC_PARSE_CODE_IS_SEQUENCE_HEADER(x) ((x) == 0x00)
#define DIRAC_PARSE_CODE_IS_AUXILIARY_DATA(x) ((x) == 0x20)					
#define DIRAC_PARSE_CODE_IS_PICTURE(x) ((x) & 0x8)
#define DIRAC_PARSE_CODE_IS_END_OF_SEQUENCE(x) ((x) == 0x10)

						dirac_enc_data_t enc_data;
						
						const int first_parse_code = buf[4];
						
						if(DIRAC_PARSE_CODE_IS_SEQUENCE_HEADER(first_parse_code) && tack_on_header)
						{
							const int first_buf_len = (buf[5] << 24) + (buf[6] << 16) + (buf[7] << 8) + buf[8];
						
							if(sequence_header == NULL)
							{
								sequence_header = (unsigned char *)malloc(first_buf_len);
								
								assert(sequence_header != NULL);
								
								sequence_header_size = first_buf_len;
							}
							else
							{
								assert(sequence_header_size == first_buf_len);
							}
							
							memcpy(sequence_header, buf, sequence_header_size);
							
							
							// includes sequence header, copy whole thing
							enc_data.size = encoder->enc_buf.size;
							enc_data.buffer = (unsigned char *)malloc(enc_data.size);
							
							memcpy(enc_data.buffer, encoder->enc_buf.buffer, encoder->enc_buf.size);
						}
						else
						{
							// no sequence header, have to add it in
							enc_data.size = sequence_header_size + encoder->enc_buf.size;
							enc_data.buffer = (unsigned char *)malloc(enc_data.size);
							
							if(sequence_header_size > 0)
								memcpy(enc_data.buffer, sequence_header, sequence_header_size);
							
							memcpy(enc_data.buffer + sequence_header_size, encoder->enc_buf.buffer, encoder->enc_buf.size);
						}
						
						out.push(enc_data);
						
						
						
						while(buf_size > 0)
						{
							const int buf_len = (buf[5] << 24) + (buf[6] << 16) + (buf[7] << 8) + buf[8];

							const int parse_code = buf[4];
							
#define DIRAC_PARSE_HEADER_SIZE (4+1+4+4)
							assert(buf_len >= DIRAC_PARSE_HEADER_SIZE);

							if( DIRAC_PARSE_CODE_IS_SEQUENCE_HEADER(parse_code) )
							{
								std::cout << "[[Sequence header";
							}
							else if( DIRAC_PARSE_CODE_IS_AUXILIARY_DATA(parse_code) )
							{
								std::cout << "[[Auxiliary data";
							}
							else if( DIRAC_PARSE_CODE_IS_PICTURE(parse_code) )
							{
								std::cout << "[[Picture " << frames_out;
								
								frames_out++;
							}
							else if( DIRAC_PARSE_CODE_IS_END_OF_SEQUENCE(parse_code) )
							{
								std::cout << "[[End of sequence";
								
								buf_size = 0; // buf_len will be 0, not the actual size of the buffer
							}
							
							std::cout << ", size: " << buf_len << "]]" << std::endl;
							
							buf += buf_len;
							
							buf_size -= buf_len;
						}
												
						assert(buf_size == 0);
					}
				}
				else if(state == ENC_STATE_BUFFER)
				{
					encoder_pull = false;
				}
				else if(state == ENC_STATE_EOS)
				{
					std::cout << "[[End of sequence]]" << std::endl;
				
					// store packet
					dirac_enc_data_t enc_data;
					
					enc_data.buffer = (unsigned char *)malloc(encoder->enc_buf.size);
					enc_data.size = encoder->enc_buf.size;
					
					memcpy(enc_data.buffer, encoder->enc_buf.buffer, encoder->enc_buf.size);
					
					out.push(enc_data);
					
					encoder_pull = false;
				}
				else if(state == ENC_STATE_INVALID)
				{
					std::cout << "[[INVALID STATE]]" << std::endl;
				}
				else
					std::cout << "[[UNKNOWN STATE]]" << std::endl;
			}
		}
	
		free(video_buf);
		
		if(sequence_header != NULL)
			free(sequence_header);
		
		dirac_encoder_close(encoder);
	}
	
	free(enc_data.buffer);

    return out;
}


static void
send_eos(dirac_decoder_t *decoder)
{
	unsigned char buf[DIRAC_PARSE_HEADER_SIZE];
	
	assert(DIRAC_PARSE_HEADER_SIZE == 13);
	
	buf[0] = 'B';
	buf[1] = 'B';
	buf[2] = 'C';
	buf[3] = 'D';
	
	buf[4] = 0x10; // EOS parse code
	
	buf[5] = buf[6] = buf[7] = buf[8] = 0; // size
	
	buf[9] = 0;
	buf[10] = 0;
	buf[11] = 0xEA;
	buf[12] = 0xC5;
	
	dirac_buffer(decoder, buf, buf + DIRAC_PARSE_HEADER_SIZE);
}


static void
send_eos(dirac::DiracParser &parser)
{
	unsigned char buf[DIRAC_PARSE_HEADER_SIZE];
	
	assert(DIRAC_PARSE_HEADER_SIZE == 13);
	
	buf[0] = 'B';
	buf[1] = 'B';
	buf[2] = 'C';
	buf[3] = 'D';
	
	buf[4] = 0x10; // EOS parse code
	
	buf[5] = buf[6] = buf[7] = buf[8] = 0; // size
	
	buf[9] = 0;
	buf[10] = 0;
	buf[11] = 0xEA;
	buf[12] = 0xC5;
	
	parser.SetBuffer((char *)buf, (char *)buf + DIRAC_PARSE_HEADER_SIZE);
}


static void
uncompress_frame(const dirac_enc_data_t &enc_data)
{
	dirac_decoder_t *decoder = dirac_decoder_init(true);
	
	if(decoder)
	{
		bool frame_sent = false;
		
		bool decoder_go = true;
		
		while(decoder_go)
		{
			dirac_decoder_state_t state = dirac_parse(decoder);
			
			if(state == STATE_BUFFER)
			{
				if(!frame_sent)
				{
					dirac_buffer(decoder, enc_data.buffer, enc_data.buffer + enc_data.size);
					
					frame_sent = true;
				}
				else
				{
					send_eos(decoder);
				}
			}
			else if(state == STATE_SEQUENCE)
			{
				// allocate buffers
				unsigned char *buf[3] = {NULL, NULL, NULL};
				
				buf[0] = (unsigned char *)malloc(decoder->src_params.width * decoder->src_params.height);
				buf[1] = (unsigned char *)malloc(decoder->src_params.chroma_width * decoder->src_params.chroma_height);
				buf[2] = (unsigned char *)malloc(decoder->src_params.chroma_width * decoder->src_params.chroma_height);
				
				dirac_set_buf(decoder, buf, NULL);
				
				std::cout << "[[Create buffers]]" << std::endl;
			}
			else if(state == STATE_PICTURE_AVAIL)
			{
				
			
				std::cout << "[[Frame out]]" << std::endl;
			}
			else if(state == STATE_SEQUENCE_END)
			{
				// free buffers
				for(int i = 0; i < 3; i++)
				{
					if(decoder->fbuf->buf[i])
						free(decoder->fbuf->buf[i]);
					
					decoder->fbuf->buf[i] = NULL;
				}
				
				std::cout << "[[Free buffers, Sequence End]]" << std::endl;
				
				decoder_go = false;
			}
			else
				std::cout << "[[Unknown decoder state]]" << std::endl;
		}
	
		dirac_decoder_close(decoder);
	}
}

static void
uncompress_frame_cpp(const dirac_enc_data_t &enc_data)
{
	dirac::DiracParser parser;
	
	bool frame_sent = false;
	
	bool decoder_go = true;
	
	while(decoder_go)
	{
		DecoderState state = parser.Parse();
		
		if(state == STATE_BUFFER)
		{
			if(!frame_sent)
			{
				parser.SetBuffer((char *)enc_data.buffer, (char *)enc_data.buffer + enc_data.size);
				
				std::cout << "[[Frame sent]]" << std::endl;
				
				frame_sent = true;
			}
			else
			{
				send_eos(parser);
				
				std::cout << "[[EOS sent]]" << std::endl;
			}
		}
		else if(state == STATE_PICTURE_AVAIL)
		{
			std::cout << "[[Frame out]]" << std::endl;
		}
		else if(state == STATE_SEQUENCE_END)
		{
			std::cout << "[[Sequence end]]" << std::endl;
			
			decoder_go = false;
		}
	}
}


static void
uncompress_video_byframe(packets &in)
{
	int frame_num = 0;

	while( !in.empty() )
	{
		const dirac_enc_data_t &enc_data = in.front();
		
		unsigned char *buf = enc_data.buffer;
		
		const int parse_code = buf[4];
		
		if( !DIRAC_PARSE_CODE_IS_END_OF_SEQUENCE(parse_code) )
		{
			uncompress_frame(enc_data);
			
			//uncompress_frame_cpp(enc_data);
			
			std::cout << "[[Frame " << frame_num << "]]" << std::endl;
			
			frame_num++;
		}
		
		free(enc_data.buffer);
		
		in.pop();
	}
}


template <typename BUFTYPE, int BITDEPTH>
static void
uncompress_video_normal(packets &in)
{
	// this wants a frame sequence plus EOS packet
	dirac_decoder_t *decoder = dirac_decoder_init(true);
	
	if(decoder)
	{
		int frames_in = 0;
		int frames_out = 0;
		
		bool decoder_go = true;
		
		while(decoder_go)
		{
			dirac_decoder_state_t state = dirac_parse(decoder);
			
			if(state == STATE_BUFFER)
			{
				if( !in.empty() )
				{
					const dirac_enc_data_t &enc_data = in.front();
					
					dirac_buffer(decoder, enc_data.buffer, enc_data.buffer + enc_data.size);
					
					unsigned char *buf = enc_data.buffer;
					
					const int parse_code = buf[4];
					
					if( DIRAC_PARSE_CODE_IS_END_OF_SEQUENCE(parse_code) )
					{
						std::cout << "[[EOS in]]" << std::endl;
					}
					else
						std::cout << "[[Frame " << frames_in << " in]]" << std::endl;
					
					frames_in++;
					
					free(enc_data.buffer);
					
					in.pop();
				}
				else
					std::cout << "[[No frames left!!!]]" << std::endl;
			}
			else if(state == STATE_SEQUENCE)
			{
				// allocate buffers
				unsigned char *buf[3] = {NULL, NULL, NULL};
				
				buf[0] = (unsigned char *)malloc(decoder->src_params.width * decoder->src_params.height * sizeof(BUFTYPE));
				buf[1] = (unsigned char *)malloc(decoder->src_params.chroma_width * decoder->src_params.chroma_height * sizeof(BUFTYPE));
				buf[2] = (unsigned char *)malloc(decoder->src_params.chroma_width * decoder->src_params.chroma_height * sizeof(BUFTYPE));
				
				dirac_set_buf(decoder, buf, NULL);
				
				std::cout << "[[Create buffers]]" << std::endl;
			}
			else if(state == STATE_PICTURE_AVAIL)
			{
				std::cout << "[[Frame " << frames_out << " out]]" << std::endl;
				
				PrintLine<BUFTYPE>((BUFTYPE *)decoder->fbuf->buf[0]);
				
				frames_out++;
			}
			else if(state == STATE_SEQUENCE_END)
			{
				// free buffers
				for(int i = 0; i < 3; i++)
				{
					if(decoder->fbuf->buf[i])
						free(decoder->fbuf->buf[i]);
					
					decoder->fbuf->buf[i] = NULL;
				}
				
				std::cout << "[[Free buffers, Sequence End]]" << std::endl;
				
				decoder_go = false;
			}
			else
				std::cout << "[[Unknown decoder state]]" << std::endl;
		}
		
		dirac_decoder_close(decoder);
	}
}


static void
uncompress_video_normal_cpp(packets &in)
{
	// this wants a frame sequence plus EOS packet
	dirac::DiracParser parser;
	
	int frames_in = 0;
	int frames_out = 0;
	
	bool decoder_go = true;
	
	while(decoder_go)
	{
		DecoderState state = parser.Parse();
		
		if(state == STATE_BUFFER)
		{
			if( !in.empty() )
			{
				const dirac_enc_data_t &enc_data = in.front();
				
				parser.SetBuffer((char *)enc_data.buffer, (char *)enc_data.buffer + enc_data.size);
				
				unsigned char *buf = enc_data.buffer;
				
				const int parse_code = buf[4];
				
				if( DIRAC_PARSE_CODE_IS_END_OF_SEQUENCE(parse_code) )
				{
					std::cout << "[[EOS in]]" << std::endl;
				}
				else
					std::cout << "[[Frame " << frames_in << " in]]" << std::endl;
				
				frames_in++;
				
				free(enc_data.buffer);
				
				in.pop();
			}
			else
				std::cout << "[[No frames left!!!]]" << std::endl;
		}
		else if(state == STATE_SEQUENCE)
		{
			// Parser will do it, right?
		}
		else if(state == STATE_PICTURE_AVAIL)
		{
			const dirac::Picture *pic = parser.GetNextPicture();
			
			assert(pic->GetPparams().Xl() == width);
			assert(pic->GetPparams().Yl() == height);
					
			std::cout << "[[Frame " << frames_out << " out]]" << std::endl;
			
			const dirac::PicArray &y_comp = pic->Data(dirac::Y_COMP);
			
			PrintLine(y_comp);
			
			frames_out++;
		}
		else if(state == STATE_SEQUENCE_END)
		{
			std::cout << "[[Sequence End]]" << std::endl;
			
			decoder_go = false;
		}
		else
			std::cout << "[[Unknown decoder state]]" << std::endl;
	}
}


static void
schro_uncompress_video_normal(packets &in, int bit_depth)
{
	schro_init();
	
	SchroDecoder *decoder = schro_decoder_new();
	
	if(decoder)
	{
		int frames_in = 0;
		int frames_out = 0;
		
		bool decoder_go = true;
		
		while(decoder_go)
		{
			int state = schro_decoder_wait(decoder);
			
			if(state == SCHRO_DECODER_FIRST_ACCESS_UNIT)
			{
				SchroVideoFormat *format = schro_decoder_get_video_format(decoder);
				
				if(format)
				{
					assert(format->width == width);
					assert(format->height == height);
					
					assert(format->chroma_format == SCHRO_CHROMA_444);
					
					free(format);
				}
			}
			else if(state == SCHRO_DECODER_NEED_BITS)
			{
				if( !in.empty() )
				{
					const dirac_enc_data_t &enc_data = in.front();
					
					unsigned char *buf = enc_data.buffer;
					ptrdiff_t buf_pos = 0;

					while(buf_pos <= (enc_data.size - SCHRO_PARSE_HEADER_SIZE) && buf[0] == 'B' && buf[1] == 'B' && buf[2] == 'C' && buf[3] == 'D')
					{
						unsigned int parse_code = buf[4];
						
						const int buf_len = (buf[5] << 24) + (buf[6] << 16) + (buf[7] << 8) + buf[8];
						
						if(buf_len < SCHRO_PARSE_HEADER_SIZE)
							break;
						
						SchroBuffer *schro_buf = schro_buffer_new_with_data(buf, buf_len);
						
						schro_decoder_push(decoder, schro_buf);
						
						buf += buf_len;
						buf_pos += buf_len;
					}
					
					
					buf = enc_data.buffer;
					
					const int parse_code = buf[4];
					
					if( SCHRO_PARSE_CODE_IS_END_OF_SEQUENCE(parse_code) )
					{
						std::cout << "[[EOS in]]" << std::endl;
					}
					else
						std::cout << "[[Frame " << frames_in << " in]]" << std::endl;
					
					frames_in++;
					
					free(enc_data.buffer);
					
					in.pop();
				}
				else
				{
					std::cout << "[[No frames left, call EOS]]" << std::endl;
					
					schro_decoder_push_end_of_stream(decoder);
				}
			}
			else if(state == SCHRO_DECODER_NEED_FRAME)
			{
				SchroFrameFormat frame_format = SCHRO_FRAME_FORMAT_U8_444;
				
				if(bit_depth > 8)
					frame_format = SCHRO_FRAME_FORMAT_S16_444;
					
				if(bit_depth > 15)
					frame_format = SCHRO_FRAME_FORMAT_S32_444;
					
				SchroFrame *schro_frame = schro_frame_new_and_alloc(NULL, frame_format, width, height);
				
				schro_decoder_add_output_picture(decoder, schro_frame);
			}
			else if(state == SCHRO_DECODER_OK)
			{
				//SchroTag *tag = schro_decoder_get_picture_tag(_decoder);
				
				SchroFrame *schro_frame = schro_decoder_pull(decoder);
				
				if(schro_frame != NULL)
				{
					std::cout << "[[Frame " << frames_out++ << " out]]" << std::endl;
				
					const SchroFrameFormat frame_format = schro_frame->components[0].format;
					
					if(frame_format == SCHRO_FRAME_FORMAT_DEPTH_S16)
					{
						PrintLine<short>((short *)schro_frame->components[0].data);
					}
					else if(frame_format == SCHRO_FRAME_FORMAT_DEPTH_S32)
					{
						PrintLine<int>((int *)schro_frame->components[0].data);
					}
					else
					{
						assert(frame_format == SCHRO_FRAME_FORMAT_DEPTH_U8);
					
						PrintLine<unsigned char>((unsigned char *)schro_frame->components[0].data);
					}
					
					schro_frame_unref(schro_frame);
				}
				else
					assert(false);
			}
			else if(state == SCHRO_DECODER_EOS)
			{
				std::cout << "[[EOS out]]" << std::endl;
			
				decoder_go = false;
			}
			else if(state == SCHRO_DECODER_ERROR)
			{
				assert(false);
				
				decoder_go = false;
			}
		}
		
		schro_decoder_free(decoder);
	}
}


int main(int argc, char * const argv[])
{
	packets compressed = compress_video<unsigned char, 8>();
	
	std::cout << std::endl << "======================" << std::endl;
	
	schro_uncompress_video_normal(compressed, 8); // works
	
	std::cout << std::endl << "======================" << std::endl;
	
	packets compressed2 = compress_video<short, 10>();
	
	std::cout << std::endl << "======================" << std::endl;
	
	schro_uncompress_video_normal(compressed2, 10); // no work
	
	std::cout << std::endl << "======================" << std::endl;
	
	packets compressed3 = compress_video<short, 10>();
	
	std::cout << std::endl << "======================" << std::endl;
	
	uncompress_video_normal<short, 10>(compressed3); // works

	return 0;
}