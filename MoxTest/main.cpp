/*
 *  main.cpp
 *  MOX_AE
 *
 *  Created by Brendan Bolles on 8/26/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/FrameBuffer.h>

#include <iostream>

#include <math.h>

using namespace MoxFiles;

static FrameBufferPtr
MakeRGBRamp()
{
	const int width = 256;
	const int height = 1;
	
	FrameBufferPtr frame = new FrameBuffer(width, height);
	
	const size_t subpixel_size = sizeof(unsigned char);
	const size_t pixel_size = subpixel_size * 3;
	const size_t rowbytes = pixel_size * width;
	const size_t data_size = rowbytes * height;
	
	DataChunkPtr data = new DataChunk(data_size);
	
	frame->attachData(data);
	
	unsigned char *origin = data->Data;
	
	const char *chan[3] = {"R", "G", "B"};
	
	for(int i = 0; i < 3; i++)
	{
		frame->insert(chan[i], Slice(MoxFiles::UINT8, (char *)origin + (i * subpixel_size), pixel_size, rowbytes));
	}
	
	unsigned char *pix = origin;
	
	for(int x = 0; x < width; x++)
	{
		for(int c = 0; c < 3; c++)
		{
			//*pix++ = x;
			if(c == 0)
				*pix++ = 255;
			else
				*pix++ = 0;
		}
	}
	
	return frame;
}

static FrameBufferPtr
MakeYCbCrRamp()
{
	const int width = 256;
	const int height = 1;
	
	FrameBufferPtr frame = new FrameBuffer(width, height);
	
	const size_t subpixel_size = sizeof(unsigned char);
	const size_t pixel_size = subpixel_size * 3;
	const size_t rowbytes = pixel_size * width;
	const size_t data_size = rowbytes * height;
	
	DataChunkPtr data = new DataChunk(data_size);
	
	frame->attachData(data);
	
	unsigned char *origin = data->Data;
	
	const char *chan[3] = {"Y", "Cb", "Cr"};
	
	for(int i = 0; i < 3; i++)
	{
		frame->insert(chan[i], Slice(MoxFiles::UINT8, (char *)origin + (i * subpixel_size), pixel_size, rowbytes));
	}
	
	unsigned char *pix = origin;
	
	for(int x = 0; x < width; x++)
	{
		*pix++ = x;
		*pix++ = 128;
		*pix++ = 128;
	}
	
	return frame;
}


static bool
FrameBufferYUVTest()
{
	bool success = true;
	
	FrameBufferPtr start_ramp = MakeRGBRamp();
	FrameBufferPtr yuv_ramp = MakeYCbCrRamp();
	FrameBufferPtr end_ramp = MakeRGBRamp();
	
	yuv_ramp->coefficients() = FrameBuffer::Rec601;
	
	yuv_ramp->copyFromFrame(*start_ramp);
	
	end_ramp->copyFromFrame(*yuv_ramp);
	
	
	Slice &R_start = (*start_ramp)["R"];
	Slice &G_start = (*start_ramp)["G"];
	Slice &B_start = (*start_ramp)["B"];
	
	Slice &Y_mid = (*yuv_ramp)["Y"];
	Slice &Cb_mid = (*yuv_ramp)["Cb"];
	Slice &Cr_mid = (*yuv_ramp)["Cr"];
	
	Slice &R_end = (*end_ramp)["R"];
	Slice &G_end = (*end_ramp)["G"];
	Slice &B_end = (*end_ramp)["B"];
	
	unsigned char *R_s = (unsigned char *)R_start.base;
	unsigned char *G_s = (unsigned char *)G_start.base;
	unsigned char *B_s = (unsigned char *)B_start.base;
	
	unsigned char *Y_m = (unsigned char *)Y_mid.base;
	unsigned char *Cb_m = (unsigned char *)Cb_mid.base;
	unsigned char *Cr_m = (unsigned char *)Cr_mid.base;
	
	unsigned char *R_e = (unsigned char *)R_end.base;
	unsigned char *G_e = (unsigned char *)G_end.base;
	unsigned char *B_e = (unsigned char *)B_end.base;
	
	const int R_s_step = R_start.xStride / sizeof(unsigned char);
	const int G_s_step = G_start.xStride / sizeof(unsigned char);
	const int B_s_step = B_start.xStride / sizeof(unsigned char);
	
	const int Y_m_step = Y_mid.xStride / sizeof(unsigned char);
	
	const int R_e_step = R_end.xStride / sizeof(unsigned char);
	const int G_e_step = G_end.xStride / sizeof(unsigned char);
	const int B_e_step = B_end.xStride / sizeof(unsigned char);
	
	for(int x = 0; x < 256; x++)
	{
		//std::cout << "{" << (int)*R_s << "} -> {" << (int)*Y_m << "} -> {" << (int)*R_e << "}" << std::endl;
	
		if(abs((int)*R_s - (int)*R_e) > 1)
			success = false;
		
		if(abs((int)*G_s - (int)*G_e) > 1)
			success = false;

		if(abs((int)*B_s - (int)*B_e) > 1)
			success = false;
		
		R_s += R_s_step;
		G_s += R_s_step;
		B_s += R_s_step;
		
		Y_m += Y_m_step;
		
		R_e += R_e_step;
		G_e += G_e_step;
		B_e += B_e_step;
	}
	
	return success;
}

template <typename T, int MAX>
static bool
YCgCoTest()
{
	// This test fails.  You can not perform RGB -> YCgCo -> RGB at 8-bit without loss.  You can
	// do it losslessly if you allocate two more bits for YCgCo.
	bool success = true;
	
	const float norm = (MAX + 1) / 2;

	for(int r = 0; r <= MAX; r++)
	{
		for(int g = 0; g <= MAX; g++)
		{
			for(int b = 0; b <= MAX; b++)
			{
				const T Y = ((float)r / 4.0) + ((float)g / 2.0) + ((float)b / 4.0) + 0.5;
				const T Cg =  ((float)r / -4.0) + ((float)g / 2.0) + ((float)b / -4.0) + norm + 0.5;
				const T Co =  ((float)r / 2.0) + ((float)b / -2.0) + norm + 0.5;
				
				const T R = (int)Y - ((int)Cg - (int)norm) + ((int)Co - (int)norm);
				const T G = (int)Y + ((int)Cg - (int)norm);
				const T B = (int)Y - ((int)Cg - (int)norm) - ((int)Co - (int)norm);
				
				// ------
				/*
				// another way to do it
				//float Co_f = (float)r - (float)b;
				//float temp = (float)b + (Co_f / 2.0);
				//float Cg_f = (float)g - temp;
				//float Y_f = temp + (Cg_f / 2.0);
				
				float Y_f = ((float)r / 4.0) + ((float)g / 2.0) + ((float)b / 4.0);
				float Cg_f =  ((float)r / -4.0) + ((float)g / 2.0) + ((float)b / -4.0);
				float Co_f =  ((float)r / 2.0) + ((float)b / -2.0);
				
				const T Y = Y_f + 0.5;
				const T Cg =  Cg_f + norm + 0.5;
				const T Co =  Co_f + norm + 0.5;
				
				//Y_f = Y;
				//Cg_f = (float)Cg - norm;
				//Co_f = (float)Co - norm;
				
				float temp2 = Y_f - Cg_f;
				float R_f = temp2 + Co_f;
				float G_f = Y_f + Cg_f;
				float B_f = temp2 - Co_f;
				
				const T R = R_f + 0.5;
				const T G = G_f + 0.5;
				const T B = B_f + 0.5;
				*/
				// ---------
				/*
				// cool alternative YCoCg24 that actually works found here
				// http://stackoverflow.com/questions/10566668/lossless-rgb-to-ycbcr-transformation
				
				const T Co = ((int)b - (int)r) % 0x100;
				const int temp =  ((int)r + ((int)Co >> 1)) % 0x100;
				const T Cg = ((int)temp - (int)g) % 0x100;
				const T Y = ((int)g + ((int)Cg >> 1)) % 0x100;
				
				const T G = ((int)Y - ((int)Cg >> 1)) % 0x100;
				const int temp2 = ((int)G + (int)Cg) % 0x100;
				const T R = ((int)temp2 - ((int)Co >> 1)) % 0x100;
				const T B = ((int)R + (int)Co) % 0x100;
				
				seems similar to YCoCg-R here
				// http://research.microsoft.com/pubs/102040/2008_ColorTransforms_MalvarSullivanSrinivasan.PDF
				*/
				// -----------
				/*
				// JPEG 2000 reversible color transform
				// https://en.wikipedia.org/wiki/JPEG_2000#Color_components_transformation
				// Doesn't seem totally reversible without more bits
				
				const T Y = (((float)r + (2.0 * (float)g) + (float)b) / 4.0) + 0.5;
				const int Cb = (int)b - (int)g;
				const int Cr = (int)r - (int)g;
				
				const int Cg = Cb; // for display only
				const int Co = Cr;
				
				const T G = (float)Y - (((float)Cb + (float)Cr) / 4.0) + 0.5;
				const T R = Cr + G;
				const T B = Cb + G;
				*/
				
				std::cout << "{" << (int)r << ", " << (int)g << ", " << (int)b << "} -> ";
				std::cout << "{" << (int)Y << ", " << (int)Cg << ", " << (int)Co << "} -> ";
				std::cout << "{" << (int)R << ", " << (int)G << ", " << (int)B << "}";
				
				if(r != R || g != G || b != B)
				{
					std::cout << " NO MATCH";
				
					success = false;
				}
				
				std::cout << std::endl;
			}
		}
	}
	
	return success;
}


int main(int argc, char * const argv[])
{
	bool success = true;
	
	try
	{
		std::cout << "FrameBufferYUVTest...";
		const bool yuv_test = FrameBufferYUVTest();
		std::cout << (yuv_test ? "success" : "failed") << std::endl;
		if(!yuv_test)
			success = false;
		
		//std::cout << "YCgCoTest...";
		//const bool ycgco_test = YCgCoTest<unsigned char, 255>();
		//std::cout << (ycgco_test ? "success" : "failed") << std::endl;
		//if(!ycgco_test)
		//	success = false;
	}
	catch(std::exception &e)
	{
		std::cout << "Exception thrown: " << e.what() << std::endl;
		
		success = false;
	}
	
	std::cout << std::endl;
	
	if(success)
		std::cout << "Tests successful!" << std::endl;
	else
		std::cout << "Tests failed :`(" << std::endl;
	
    return (success ? 0 : -1);
}