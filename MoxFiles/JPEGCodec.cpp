/*
 *  JPEGCodec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 11/15/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#include <MoxFiles/JPEGCodec.h>

#include <MoxFiles/MemoryFile.h>

#include <iostream>

#include <assert.h>

#include "jpeglib.h"

#include "jerror.h"

namespace MoxFiles
{

JPEGCodec::JPEGCodec(const Header &header, const ChannelList &channels) :
	VideoCodec(header, channels),
	_descriptor(header.frameRate(), header.width(), header.height(), MoxMxf::VideoDescriptor::VideoCodecJPEG)
{
	setWindows(_descriptor, header);
	
	assert(channels.size() == 3);
	
	const Channel *r_channel = channels.findChannel("R");
	
	if(r_channel)
	{
		assert(r_channel->type == UINT8);
	}
	else
		assert(false);
	
	
	MoxMxf::RGBADescriptor::RGBALayout layout;
	
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('R', 8));
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('G', 8));
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('B', 8));
	
	_descriptor.setPixelLayout(layout);
	
	
	_quality = (isLossless(header) ? 100 : getQuality(header));
}


JPEGCodec::JPEGCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<const MoxMxf::RGBADescriptor &>(descriptor))
{
	assert(_descriptor.getVideoCodec() == MoxMxf::VideoDescriptor::VideoCodecJPEG);

	const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = _descriptor.getPixelLayout();
	
	assert(pixelLayout.size() == 3 && pixelLayout.at(0).depth == 8);
	
	channels.insert("R", UINT8);
	channels.insert("G", UINT8);
	channels.insert("B", UINT8);
}


JPEGCodec::~JPEGCodec()
{

}


static void my_error_exit(j_common_ptr cinfo)
{
	std::cout << "my_error_exit" << std::endl;
	
	throw MoxMxf::ArgExc("fatal libjpeg error");
}

static void my_emit_message(j_common_ptr cinfo, int msg_level)
{
	
	struct jpeg_error_mgr *err = cinfo->err;
	
	if(msg_level < 0)
	{
		// warning
		std::cout << "my_emit_message warning: " << msg_level << std::endl;
		
		if(err->num_warnings == 0 || err->trace_level >= 3)
		{
			(*err->output_message) (cinfo);
		}
	  
		err->num_warnings++;
	}
	else
	{
		// trace message
		if(err->trace_level >= msg_level)
		{
			(*err->output_message) (cinfo);
		}
	}
}

static void my_output_message(j_common_ptr cinfo)
{
	std::cout << "my_output_message" << std::endl;
}

static void my_format_message(j_common_ptr cinfo, char *buffer)
{
	std::cout << "my_format_message" << std::endl;
	
	struct jpeg_error_mgr *err = cinfo->err;
	
	const int msg_code = err->msg_code;
	
	const char *msgtext = NULL;
	
	if(msg_code > 0 && msg_code <= err->last_jpeg_message)
	{
		msgtext = err->jpeg_message_table[msg_code];
	}
	else if (err->addon_message_table != NULL &&
             msg_code >= err->first_addon_message &&
             msg_code <= err->last_addon_message)
	{
		msgtext = err->addon_message_table[msg_code - err->first_addon_message];
	}
	
	assert(msgtext != NULL);
}

static void my_reset_error_mgr(j_common_ptr cinfo)
{
	//std::cout << "my_reset_error_mgr" << std::endl;
	
	cinfo->err->num_warnings = 0;
	cinfo->err->msg_code = 0;
}

static const char * const jpeg_std_message_table[] = {
#include "jerror.h"
  NULL
};


typedef struct
{
  struct jpeg_destination_mgr pub; /* public fields */

  MemoryFile *outfile;                /* target stream */
  
  JOCTET *buffer;
  size_t bufferSize;
  
} my_destination_mgr;

static void my_init_destination(j_compress_ptr cinfo)
{
	my_destination_mgr *mgr = (my_destination_mgr *)cinfo->dest;
	
	const size_t bufSize = 4096;
	
	mgr->buffer = (JOCTET *)malloc(bufSize);
	mgr->bufferSize = bufSize / sizeof(JOCTET);
	
	if(mgr->buffer == NULL)
		throw MoxMxf::NullExc("out of memory");
	
	mgr->pub.next_output_byte = mgr->buffer;
	mgr->pub.free_in_buffer = mgr->bufferSize;
}

static boolean my_empty_output_buffer(j_compress_ptr cinfo)
{
	my_destination_mgr *mgr = (my_destination_mgr *)cinfo->dest;

	const MoxMxf::UInt64 wrote = mgr->outfile->FileWrite(mgr->buffer, mgr->bufferSize);
	
	if(wrote != mgr->bufferSize)
	{
		ERREXIT(cinfo, JERR_FILE_WRITE);
	}
	
	mgr->pub.next_output_byte = mgr->buffer;
	mgr->pub.free_in_buffer = mgr->bufferSize;
	
	return TRUE;
}

static void my_term_destination(j_compress_ptr cinfo)
{
	my_destination_mgr *mgr = (my_destination_mgr *)cinfo->dest;
	
	const size_t leftToWrite = (mgr->bufferSize - mgr->pub.free_in_buffer);
	
	if(leftToWrite > 0)
	{
		const MoxMxf::UInt64 wrote = mgr->outfile->FileWrite(mgr->buffer, leftToWrite);
		
		if(wrote != leftToWrite)
		{
			ERREXIT(cinfo, JERR_FILE_WRITE);
		}
	}
}


void
JPEGCodec::compress(const FrameBuffer &frame)
{
	struct jpeg_error_mgr jerr;
	
	jerr.error_exit = my_error_exit;
	jerr.emit_message = my_emit_message;
	jerr.output_message = my_output_message;
	jerr.format_message = my_format_message;
	jerr.reset_error_mgr = my_reset_error_mgr;
	
	jerr.trace_level = 0;
	jerr.num_warnings = 0;
	jerr.msg_code = 0;
	
	jerr.jpeg_message_table = jpeg_std_message_table;
	jerr.last_jpeg_message = (int) JMSG_LASTMSGCODE - 1;
	
	jerr.addon_message_table = NULL;
	jerr.first_addon_message = 0;
	jerr.last_jpeg_message = 0;
	
	
	
	struct jpeg_compress_struct cinfo;
	
	cinfo.err = &jerr;
	
	jpeg_create_compress(&cinfo);
	
	
	my_destination_mgr mgr;
	
	mgr.outfile = new MemoryFile;
	mgr.buffer = NULL;
	mgr.bufferSize = 0;
	
	mgr.pub.init_destination = my_init_destination;
	mgr.pub.empty_output_buffer = my_empty_output_buffer;
	mgr.pub.term_destination = my_term_destination;
	
	
	cinfo.dest = (jpeg_destination_mgr *)&mgr;
	
	
	const Box2i dataW = dataWindow();
	
	const int width = (dataW.max.x - dataW.min.x + 1);
	const int height = (dataW.max.y - dataW.min.y + 1);
	
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	
	jpeg_set_defaults(&cinfo);
	
	
	jpeg_set_quality(&cinfo, _quality, TRUE);
	
	
	bool success = true;
	
	try
	{
		jpeg_start_compress(&cinfo, TRUE);
		
		
		const size_t tempPixelSize = (3 * PixelSize(UINT8));
		const size_t tempRowbytes = (tempPixelSize * width);
		const size_t tempBufSize = (tempRowbytes * height);
		
		DataChunk dataChunk(tempBufSize);
		
		char *tempBuffer = (char *)dataChunk.Data;
		
		FrameBuffer tempFrameBuffer(dataW);
		
		tempFrameBuffer.insert("R", Slice(UINT8, &tempBuffer[0], tempPixelSize, tempRowbytes));
		tempFrameBuffer.insert("G", Slice(UINT8, &tempBuffer[1], tempPixelSize, tempRowbytes));
		tempFrameBuffer.insert("B", Slice(UINT8, &tempBuffer[2], tempPixelSize, tempRowbytes));
		
		tempFrameBuffer.copyFromFrame(frame);
		
		
		JSAMPARRAY scanlines = (JSAMPARRAY)malloc(height * sizeof(JSAMPROW));
		
		if(scanlines == NULL)
			throw MoxMxf::NullExc("out of memory");
		
		for(int y=0; y < height; y++)
		{
			scanlines[y] = (JSAMPROW)(tempBuffer + (y * tempRowbytes));
		}
		
		
		const JDIMENSION linesWrote = jpeg_write_scanlines(&cinfo, scanlines, height);
		
		assert(linesWrote == height);
		
		
		jpeg_finish_compress(&cinfo);
		
		
		assert(jerr.msg_code == 0);
		
		storeData( mgr.outfile->getDataChunk() );
		
		
		free(scanlines);
	}
	catch(...)
	{
		success = false;
	}
	
	
	jpeg_destroy_compress(&cinfo);
	
	
	if(mgr.buffer != NULL)
		free(mgr.buffer);
	
	delete mgr.outfile;
	
	
	if(!success)
		throw MoxMxf::ArgExc("JPEG compression error");
}


typedef struct
{
  struct jpeg_source_mgr pub; /* public fields */

  MemoryFile *infile;                /* target stream */
  
  JOCTET *buffer;
  size_t bufferSize;
  
} my_source_mgr;

static void my_init_source(j_decompress_ptr cinfo)
{
	my_source_mgr *mgr = (my_source_mgr *)cinfo->src;
	
	mgr->infile->FileSeek(0);
}

static boolean my_fill_input_buffer(j_decompress_ptr cinfo)
{
	my_source_mgr *mgr = (my_source_mgr *)cinfo->src;
	
	MoxMxf::UInt64 bytesRead = mgr->infile->FileRead(mgr->buffer, mgr->bufferSize);
	
	if(bytesRead == 0)
	{
		if(mgr->infile->FileSize() == 0)
			ERREXIT(cinfo, JERR_INPUT_EMPTY);
		
		WARNMS(cinfo, JWRN_JPEG_EOF);
		
		mgr->buffer[0] = 0xFF;
		mgr->buffer[0] = JPEG_EOI;
		bytesRead = 2;
	}
	
	mgr->pub.next_input_byte = mgr->buffer;
	mgr->pub.bytes_in_buffer = bytesRead;
	
	return TRUE;
}

static void my_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	struct jpeg_source_mgr *mgr = cinfo->src;
	
	if(num_bytes > 0)
	{
		while(num_bytes > mgr->bytes_in_buffer)
		{
			num_bytes -= mgr->bytes_in_buffer;
			
			(*mgr->fill_input_buffer)(cinfo);
		}
		
		mgr->next_input_byte += num_bytes;
		mgr->bytes_in_buffer -= num_bytes;
	}
	else
		assert(false); // why bother calling this?
}

static void my_term_source(j_decompress_ptr cinfo)
{
	//my_source_mgr *mgr = (my_source_mgr *)cinfo->src;
}


void
JPEGCodec::decompress(const DataChunk &data)
{
	struct jpeg_error_mgr jerr;
	
	jerr.error_exit = my_error_exit;
	jerr.emit_message = my_emit_message;
	jerr.output_message = my_output_message;
	jerr.format_message = my_format_message;
	jerr.reset_error_mgr = my_reset_error_mgr;
	
	jerr.trace_level = 0;
	jerr.num_warnings = 0;
	jerr.msg_code = 0;
	
	jerr.jpeg_message_table = jpeg_std_message_table;
	jerr.last_jpeg_message = (int) JMSG_LASTMSGCODE - 1;
	
	jerr.addon_message_table = NULL;
	jerr.first_addon_message = 0;
	jerr.last_jpeg_message = 0;
	
	
	my_source_mgr mgr;
	
	mgr.infile = new MemoryFile(data);
	mgr.buffer = (JOCTET *)malloc(4096 * sizeof(JOCTET));
	mgr.bufferSize = 4096;
	mgr.pub.bytes_in_buffer = 0;
	mgr.pub.next_input_byte = NULL;
	
	if(mgr.buffer == NULL)
		throw MoxMxf::NullExc("out of memory");
	
	mgr.pub.init_source = my_init_source;
	mgr.pub.fill_input_buffer = my_fill_input_buffer;
	mgr.pub.skip_input_data = my_skip_input_data;
	mgr.pub.resync_to_restart = jpeg_resync_to_restart;
	mgr.pub.term_source = my_term_source;
	
	
	struct jpeg_decompress_struct cinfo;
	
	cinfo.err = &jerr;
	
	jpeg_create_decompress(&cinfo);
	
	
	cinfo.src = (jpeg_source_mgr *)&mgr;
	
	
	bool success = true;
	
	try
	{
		const int status = jpeg_read_header(&cinfo, TRUE);
		
		if(status == JPEG_HEADER_OK)
		{
			jpeg_start_decompress(&cinfo);
			
			const JDIMENSION width = cinfo.image_width;
			const JDIMENSION height = cinfo.image_height;
			
			assert(cinfo.num_components == 3);
			assert(cinfo.out_color_space == JCS_RGB);
			
			
			const size_t pixelSize = (3 * PixelSize(UINT8));
			const size_t rowBytes = (width * pixelSize);
			const size_t bufSize = (height * rowBytes);
			
			DataChunkPtr frameData = new DataChunk(bufSize);
			
			char *buf = (char *)frameData->Data;
			
			const Box2i dataW = dataWindow();
			
			assert(width == (dataW.max.x - dataW.min.x + 1));
			assert(height == (dataW.max.y - dataW.min.y + 1));
			assert(dataW.min.x == 0);
			assert(dataW.min.y == 0);
				
			FrameBufferPtr frameBuffer = new FrameBuffer(dataW);
			
			frameBuffer->insert("R", Slice(UINT8, &buf[0], pixelSize, rowBytes));
			frameBuffer->insert("G", Slice(UINT8, &buf[1], pixelSize, rowBytes));
			frameBuffer->insert("B", Slice(UINT8, &buf[2], pixelSize, rowBytes));
			
			frameBuffer->attachData(frameData);
			
			
			JSAMPARRAY scanlines = (JSAMPARRAY)malloc(height * sizeof(JSAMPROW));
			
			if(scanlines == NULL)
				throw MoxMxf::NullExc("out of memory");
			
			for(int y=0; y < height; y++)
			{
				scanlines[y] = (JSAMPROW)(buf + (y * rowBytes));
			}
			
			
			JDIMENSION linesRead = 0;
			
			while(linesRead < height)
			{
				linesRead += jpeg_read_scanlines(&cinfo, &scanlines[linesRead], height - linesRead);
			}
			
			
			free(scanlines);
			
			jpeg_finish_decompress(&cinfo);
			
			
			storeFrame(frameBuffer);
		}
		else
			throw MoxMxf::ArgExc("Error reading header");
	}
	catch(...)
	{
		success = false;
	}
	
	
	jpeg_destroy_decompress(&cinfo);
	
	
	free(mgr.buffer);
	
	delete mgr.infile;
	
	
	if(!success)
		throw MoxMxf::ArgExc("JPEG decompression error");
}


bool
JPEGCodecInfo::canCompressType(PixelType pixelType) const
{
	return (pixelType == UINT8);
}


ChannelCapabilities
JPEGCodecInfo::getChannelCapabilites() const
{
	return (Channels_RGB);
}


VideoCodec * 
JPEGCodecInfo::createCodec(const Header &header, const ChannelList &channels) const
{
	return new JPEGCodec(header, channels);
}

VideoCodec * 
JPEGCodecInfo::createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const
{
	return new JPEGCodec(descriptor, header, channels);
}


} // namespace
