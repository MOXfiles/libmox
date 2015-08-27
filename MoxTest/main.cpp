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
			*pix++ = x;
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
	
	const int Y_m_step = Y_mid.xStride / sizeof(unsigned char);
	
	const int R_e_step = R_end.xStride / sizeof(unsigned char);
	
	for(int x = 0; x < 256; x++)
	{
		//std::cout << "{" << (int)*R_s << "} -> {" << (int)*Y_m << "} -> {" << (int)*R_e << "}" << std::endl;
	
		if(abs((int)*R_s - (int)*R_e) > 1)
			success = false;
			
		R_s += R_s_step;
		
		Y_m += Y_m_step;
		
		R_e += R_e_step;
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