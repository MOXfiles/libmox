/*
 *  UncompressedCodec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/22/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/UncompressedVideoCodec.h>

#include <MoxFiles/Thread.h>

namespace MoxFiles
{

static unsigned int
PixelLayoutDepth(PixelType type)
{
	// see SMPTE 377M E.2.46
	switch(type)
	{
		case MoxFiles::UINT8:
			return 8;
		
		case MoxFiles::UINT10:
			assert(false); // not yet
			return 10;
			
		case MoxFiles::UINT12:
			assert(false); // not yet
			return 12;
			
		case MoxFiles::UINT16:
			return 16;
			
		case MoxFiles::UINT16A:
			assert(false); // not storing Adobe 16-bit
			return 16;
		
		case MoxFiles::UINT32:
			assert(false);
			return 32;
		
		case MoxFiles::HALF:
			assert(false); // uncompressed MXF doesn't know about half
			return 253;
		
		case MoxFiles::FLOAT:
			return 254;
	}
	
	throw MoxMxf::ArgExc("Unknown pixel type");
}

static unsigned int
PixelLayoutBits(PixelType type)
{
	const unsigned int depth = PixelLayoutDepth(type);
	
	return (depth == 254 ? 32 :
			depth == 253 ? 16 :
			depth);
}


static PixelType
PixelTypeFromBits(unsigned int bit_depth)
{
	switch(bit_depth)
	{
		case 8:
			return MoxFiles::UINT8;
		
		case 10:
			return MoxFiles::UINT10;
		
		case 12:
			return MoxFiles::UINT12;
		
		case 16:
			return MoxFiles::UINT16;
		
		case 32:
			assert(false);
			return MoxFiles::UINT32;
		
		case 253:
			assert(false); // no standard for this yet
			return MoxFiles::HALF;
			
		case 254:
			return MoxFiles::FLOAT;
	}
	
	throw MoxMxf::ArgExc("Unknown pixel type");
}


UncompressedVideoCodec::UncompressedVideoCodec(const Header &header, const ChannelList &channels) :
	VideoCodec(header, channels),
	_descriptor(header.frameRate(), header.width(), header.height(), MoxMxf::VideoDescriptor::VideoCodecUncompressedRGB),
	_padding(0)
{
	setWindows(_descriptor, header);
	
	
	ChannelList in_channels = channels;

	if(const Channel *r_channel = in_channels.findChannel("R"))
	{
		_channelVec.push_back( ChannelBits("R", 'R', r_channel->type) );
		
		in_channels.erase("R");
	}
	
	if(const Channel *g_channel = in_channels.findChannel("G"))
	{
		_channelVec.push_back( ChannelBits("G", 'G', g_channel->type) );
		
		in_channels.erase("G");
	}

	if(const Channel *b_channel = in_channels.findChannel("B"))
	{
		_channelVec.push_back( ChannelBits("B", 'B', b_channel->type) );
		
		in_channels.erase("B");
	}

	if(const Channel *a_channel = in_channels.findChannel("A"))
	{
		_channelVec.push_back( ChannelBits("A", 'A', a_channel->type) );
		
		in_channels.erase("A");
	}

	if(_channelVec.empty())
		throw MoxMxf::ArgExc("Need some RGBA channels please");
	
	assert(in_channels.size() == 0); // otherwise there was an extra channel in here
	
	
	MoxMxf::RGBADescriptor::RGBALayout layout;
	
	unsigned int bits_per_pixel = 0;
	
	for(std::vector<ChannelBits>::const_iterator i = _channelVec.begin(); i != _channelVec.end(); ++i)
	{
		layout.push_back( MoxMxf::RGBADescriptor::RGBALayoutItem(i->code, PixelLayoutDepth(i->type)) );
		
		bits_per_pixel += PixelLayoutBits(i->type);
	}
	
	if(bits_per_pixel % 8 != 0)
	{
		assert(false); // not really expecting this yet
	
		_padding = 8 - (bits_per_pixel % 8);
	
		layout.push_back( MoxMxf::RGBADescriptor::RGBALayoutItem('F', _padding) ); // fill
	}
	
	_descriptor.setPixelLayout(layout);
}


UncompressedVideoCodec::UncompressedVideoCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<const MoxMxf::RGBADescriptor &>(descriptor)),
	_padding(0)
{
	const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = _descriptor.getPixelLayout();
	
	const int num_channels = pixelLayout.size();
	
	for(int i = 0; i < num_channels; i++)
	{
		assert(pixelLayout[i].code == 'R' || pixelLayout[i].code == 'G' || pixelLayout[i].code == 'B' || pixelLayout[i].code == 'A');
		
		if(pixelLayout[i].code == 'F')
		{
			assert(_padding == 0);
			
			_padding = pixelLayout[i].depth;
		}
		else
		{
			const std::string chan_name(1, pixelLayout[i].code);
			
			const PixelType pixelType = PixelTypeFromBits(pixelLayout[i].depth);
			
			_channelVec.push_back(ChannelBits(chan_name, pixelLayout[i].code, pixelType));
			
			channels.insert(chan_name, Channel(pixelType));
		}
	}
}

UncompressedVideoCodec::~UncompressedVideoCodec()
{

}

/*
class CompressUncompressedRGBRowTask : public Task
{
  public:
	CompressUncompressedRGBRowTask(TaskGroup *group, char *dest_row, ptrdiff_t stride, int bit_depth, const Slice &in_slice, int width);
	~CompressUncompressedRGBRowTask() {}
	
	virtual void execute();
	
  private:
	char * const _dest_row;
	const ptrdiff_t _stride;
	const int _bit_depth;
	const Slice &_in_slice;
	const int _width;
};

CompressUncompressedRGBRowTask::CompressUncompressedRGBRowTask(TaskGroup *group, char *dest_row, ptrdiff_t stride, int bit_depth, const Slice &in_slice, int width) :
	Task(group),
	_dest_row(dest_row),
	_stride(stride),
	_bit_depth(bit_depth),
	_in_slice(in_slice),
	_width(width)
{

}

void
CompressUncompressedRGBRowTask::execute()
{
	if(_bit_depth == 8)
	{
		assert(_in_slice.type == MoxFiles::UINT8);
	
		UInt8 *out = (UInt8 *)_dest_row;
		UInt8 *in = (UInt8 *)_in_slice.base;
		
		const int out_step = stride / sizeof(UInt8);
		const int in_step = _in_slice.xStride / sizeof(UInt8);
		
		for(int x = 0; x < _width; x++)
		{
			*out = *in;
			
			out += out_step;
			in += in_step;
		}
	}
}

static void
EncodeChannel(TaskGroup *taskGroup, char *dest_origin, ptrdiff_t stride, ptrdiff_t rowbytes, int bit_depth, const Slice &in_slice, int width, int height)
{
	char *dest_row = dest_origin;
	char *source_row = in_slice.base;

	for(int y = 0; y < height; y++)
	{
		Slice slice = in_slice;
		slice.base = source_row;
	
		ThreadPool::addGlobalTask(new CompressUncompressedRGBRowTask(taskGroup, dest_row, stride, bit_depth, in_slice, width));
		
		dest_row += rowbytes;
		source_row += in_slice.yStride;
	}
}
*/

class CompressChannelBits : public Task
{
  public:
	CompressChannelBits(TaskGroup *group, char *origin, ptrdiff_t rowbytes, const std::vector<UncompressedVideoCodec::ChannelBits> &channelVec, const FrameBuffer &frame, int y);
	~CompressChannelBits() {}
	
	virtual void execute();
	
  private:
	char * const _origin;
	const ptrdiff_t _rowbytes;
	const std::vector<UncompressedVideoCodec::ChannelBits> &_channelVec;
	const FrameBuffer &_frame;
	const int _y;
	
	static void CompressChannel(char *dst_row, ptrdiff_t dst_stride, int bit_depth, const Slice &row_slice, int width);
};

CompressChannelBits::CompressChannelBits(TaskGroup *group, char *origin, ptrdiff_t rowbytes, const std::vector<UncompressedVideoCodec::ChannelBits> &channelVec, const FrameBuffer &frame, int y) :
	Task(group),
	_origin(origin),
	_rowbytes(rowbytes),
	_channelVec(channelVec),
	_frame(frame),
	_y(y)
{
	assert(frame.dataWindow().min.x == 0 && frame.dataWindow().min.y == 0);
}

void
CompressChannelBits::execute()
{
	size_t pixel_size = 0;
	
	for(int i = 0; i < _channelVec.size(); i++)
	{
		const UncompressedVideoCodec::ChannelBits &chanbit = _channelVec[i];
		
		assert(PixelBits(chanbit.type) % 8 == 0); // not handling weird bit depths yet
		
		pixel_size += PixelSize(chanbit.type);
	}
	
	
	char * _row = _origin + (_rowbytes * _y);
	
	for(int i = 0; i < _channelVec.size(); i++)
	{
		const UncompressedVideoCodec::ChannelBits &chanbit = _channelVec[i];
		
		const Slice *frame_slice = _frame.findSlice(chanbit.name);
		
		if(frame_slice)
		{
			assert(chanbit.type == frame_slice->type);
		
			Slice row_slice = *frame_slice;
			
			row_slice.base += (row_slice.yStride * _y);
			
			CompressChannel(_row, pixel_size, PixelBits(chanbit.type), row_slice, _frame.width());
		}
		else
			assert(false);
		
		_row += PixelSize(chanbit.type);
	}
}

void
CompressChannelBits::CompressChannel(char *dst_row, ptrdiff_t dst_stride, int bit_depth, const Slice &row_slice, int width)
{
	if(bit_depth == 8)
	{
		assert(row_slice.type == MoxFiles::UINT8);
		
		UInt8 *out = (UInt8 *)dst_row;
		UInt8 *in = (UInt8 *)row_slice.base;
		
		const int out_step = dst_stride / sizeof(UInt8);
		const int in_step = row_slice.xStride / sizeof(UInt8);
		
		for(int x = 0; x < width; x++)
		{
			*out = *in;
			
			out += out_step;
			in += in_step;
		}
	}
	else if(bit_depth == 16)
	{
		assert(row_slice.type == MoxFiles::UINT16);
		
		UInt8 *out = (UInt8 *)dst_row;
		UInt16 *in = (UInt16 *)row_slice.base;
		
		const int out_step = dst_stride / sizeof(UInt8);
		const int in_step = row_slice.xStride / sizeof(UInt16);
		
		for(int x = 0; x < width; x++)
		{
			out[0] = (*in & 0xff00) >> 8;
			out[1] = (*in & 0xff) >> 0;
			
			out += out_step;
			in += in_step;
		}
	}
	else if(bit_depth == 32)
	{
		assert(row_slice.type == MoxFiles::FLOAT);
		
		UInt8 *out = (UInt8 *)dst_row;
		UInt32 *in = (UInt32 *)row_slice.base;
		
		const int out_step = dst_stride / sizeof(UInt8);
		const int in_step = row_slice.xStride / sizeof(UInt32);
		
		for(int x = 0; x < width; x++)
		{
			out[0] = (*in & 0xff000000) >> 24;
			out[1] = (*in & 0xff0000) >> 16;
			out[2] = (*in & 0xff00) >> 8;
			out[3] = (*in & 0xff) >> 0;
			
			out += out_step;
			in += in_step;
		}
	}
	else
		assert(false);
}


void
UncompressedVideoCodec::compress(const FrameBuffer &frame)
{
	if(frame.dataWindow() != this->dataWindow())
		throw MoxMxf::ArgExc("Frame buffer doesn't match expected dimensions");

	bool input_matches = true;
	
	unsigned int bits_per_pixel = 0;
	
	for(std::vector<ChannelBits>::const_iterator i = _channelVec.begin(); i != _channelVec.end(); ++i)
	{
		bits_per_pixel += PixelLayoutBits(i->type);
		
		assert(PixelLayoutBits(i->type) % 8 == 0); // not dealing with weird bits yet
		
		const Slice *slice = frame.findSlice(i->name);
		
		if(slice)
		{
			if(slice->type != i->type)
				input_matches = false;
		}
		else
			assert(false);
	}
	
	bits_per_pixel += _padding;
	
	assert(bits_per_pixel % 8 == 0);
	
	assert(input_matches);
	
	
	const size_t pixel_size = (bits_per_pixel >> 3);
	const size_t rowbytes = _descriptor.getStoredWidth() * pixel_size;
	const size_t data_size = rowbytes * _descriptor.getStoredHeight();
	
	DataChunkPtr data = new DataChunk(data_size);
	
	{
		TaskGroup taskGroup;
		
		for(int y = 0; y < _descriptor.getStoredHeight(); y++)
		{
			ThreadPool::addGlobalTask(new CompressChannelBits(&taskGroup, (char *)data->Data, rowbytes, _channelVec, frame, y));
		}
	}
	
	storeData(data);
}


class DecompressChannelBits : public Task
{
  public:
	DecompressChannelBits(TaskGroup *group, const FrameBuffer &frame, const char *origin, ptrdiff_t rowbytes, const std::vector<UncompressedVideoCodec::ChannelBits> &channelVec, int y);
	~DecompressChannelBits() {}
	
	virtual void execute();
	
  private:
	const FrameBuffer &_frame;
	const char * const _origin;
	const ptrdiff_t _rowbytes;
	const std::vector<UncompressedVideoCodec::ChannelBits> &_channelVec;
	const int _y;
	
	static void DecompressChannel(const Slice &row_slice, const char *src_row, ptrdiff_t src_stride, int bit_depth, int width);
};

DecompressChannelBits::DecompressChannelBits(TaskGroup *group, const FrameBuffer &frame, const char *origin, ptrdiff_t rowbytes, const std::vector<UncompressedVideoCodec::ChannelBits> &channelVec, int y) :
	Task(group),
	_frame(frame),
	_origin(origin),
	_rowbytes(rowbytes),
	_channelVec(channelVec),
	_y(y)
{

}

void
DecompressChannelBits::execute()
{
	size_t pixel_size = 0;
	
	for(int i = 0; i < _channelVec.size(); i++)
	{
		const UncompressedVideoCodec::ChannelBits &chanbit = _channelVec[i];
		
		assert(PixelBits(chanbit.type) % 8 == 0); // not handling weird bit depths yet
		
		pixel_size += PixelSize(chanbit.type);
	}
	
	
	const char * _row = _origin + (_rowbytes * _y);
	
	for(int i = 0; i < _channelVec.size(); i++)
	{
		const UncompressedVideoCodec::ChannelBits &chanbit = _channelVec[i];
		
		const Slice *frame_slice = _frame.findSlice(chanbit.name);
		
		if(frame_slice)
		{
			assert(chanbit.type == frame_slice->type);
		
			Slice row_slice = *frame_slice;
			
			row_slice.base += (row_slice.yStride * _y);
			
			DecompressChannel(row_slice, _row, pixel_size, PixelBits(chanbit.type), _frame.width());
		}
		else
			assert(false);
		
		_row += PixelSize(chanbit.type);
	}
}

void
DecompressChannelBits::DecompressChannel(const Slice &row_slice, const char *src_row, ptrdiff_t src_stride, int bit_depth, int width)
{
	if(bit_depth == 8)
	{
		assert(row_slice.type == MoxFiles::UINT8);
		
		UInt8 *out = (UInt8 *)row_slice.base;
		UInt8 *in = (UInt8 *)src_row;
		
		const int out_step = row_slice.xStride / sizeof(UInt8);
		const int in_step = src_stride / sizeof(UInt8);
		
		for(int x = 0; x < width; x++)
		{
			*out = *in;
			
			out += out_step;
			in += in_step;
		}
	}
	else if(bit_depth == 16)
	{
		assert(row_slice.type == MoxFiles::UINT16);
		
		UInt16 *out = (UInt16 *)row_slice.base;
		UInt8 *in = (UInt8 *)src_row;
		
		const int out_step = row_slice.xStride / sizeof(UInt16);
		const int in_step = src_stride / sizeof(UInt8);
		
		for(int x = 0; x < width; x++)
		{
			*out = (in[0] << 8) | (in[1] << 0);
			
			out += out_step;
			in += in_step;
		}
	}
	else if(bit_depth == 32)
	{
		assert(row_slice.type == MoxFiles::FLOAT);
		
		UInt32 *out = (UInt32 *)row_slice.base;
		UInt8 *in = (UInt8 *)src_row;
		
		const int out_step = row_slice.xStride / sizeof(UInt32);
		const int in_step = src_stride / sizeof(UInt8);
		
		for(int x = 0; x < width; x++)
		{
			*out = (in[0] << 24) | (in[1] << 16) | (in[2] << 8) | (in[3] << 0);
			
			out += out_step;
			in += in_step;
		}
	}
	else
		assert(false);
}


void
UncompressedVideoCodec::decompress(const DataChunk &data)
{
	unsigned int bits_per_pixel = 0;
	
	for(std::vector<ChannelBits>::const_iterator i = _channelVec.begin(); i != _channelVec.end(); ++i)
	{
		bits_per_pixel += PixelLayoutBits(i->type);
		
		assert(PixelLayoutBits(i->type) % 8 == 0); // not dealing with weird bits yet
	}
	
	bits_per_pixel += _padding;
	
	assert(bits_per_pixel % 8 == 0);
	
	const size_t pixel_size = (bits_per_pixel >> 3);
	const size_t rowbytes = pixel_size * _descriptor.getStoredWidth();
	const size_t data_size = rowbytes * _descriptor.getStoredHeight();
	
	assert(data_size == data.Size);
	
	//FrameBuffer encoded_frameBuffer(_descriptor.getWidth(), _descriptor.getHeight());
	
	
	FrameBufferPtr exported_frameBuffer = new FrameBuffer(this->dataWindow());
	
	DataChunkPtr data_to_store = new DataChunk(data_size);
	
	exported_frameBuffer->attachData(data_to_store);
	
	
	//char *encoded_origin = (char *)data.Data;
	
	char *exported_origin = (char *)data_to_store->Data + _descriptor.getImageAlignmentOffset() + _descriptor.getImageStartOffset();
	
	assert(_descriptor.getImageAlignmentOffset() == 0 && _descriptor.getImageStartOffset() == 0);
	
	for(int i = 0; i < _channelVec.size(); i++)
	{
		const ChannelBits &chan = _channelVec[i];
		
		const size_t bytes_this_subpixel = (PixelLayoutBits(chan.type) >> 3);
	
		if(chan.code != 'F')
		{
			//encoded_frameBuffer.insert(chan.name, Slice(MoxFiles::UINT8, encoded_origin, pixel_size, rowbytes));
			
			//encoded_origin += bytes_this_subpixel;
			
			exported_frameBuffer->insert(chan.name, Slice(chan.type, exported_origin, pixel_size, rowbytes));
			
			exported_origin += bytes_this_subpixel;
		}
	}
	
	
	{
		TaskGroup taskGroup;
		
		for(int y = 0; y < _descriptor.getStoredHeight(); y++)
		{
			ThreadPool::addGlobalTask(new DecompressChannelBits(&taskGroup, *exported_frameBuffer, (char *)data.Data, rowbytes, _channelVec, y));
		}
	}

	
	//exported_frameBuffer->copyFromFrame(encoded_frameBuffer);
	
	storeFrame(exported_frameBuffer);
}


bool
UncompressedVideoCodecInfo::canCompressType(PixelType pixelType) const
{
	return (pixelType == UINT8 || pixelType == UINT16 || pixelType == FLOAT);
}


ChannelCapabilities
UncompressedVideoCodecInfo::getChannelCapabilites() const
{
	return (Channels_RGB | Channels_RGBA | Channels_A);
}


VideoCodec * 
UncompressedVideoCodecInfo::createCodec(const Header &header, const ChannelList &channels) const
{
	return new UncompressedVideoCodec(header, channels);
}

VideoCodec * 
UncompressedVideoCodecInfo::createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const
{
	return new UncompressedVideoCodec(descriptor, header, channels);
}


} // namespace
