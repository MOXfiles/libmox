/*
 *  JPEGXTCodec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 11/27/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#include <MoxFiles/JPEGXTCodec.h>

#include <MoxFiles/MemoryFile.h>

//#include <iostream>

#include <assert.h>

//#include "cmd/iohelpers.hpp"
//#include "cmd/main.hpp"
//#include "cmd/defaulttmoc.hpp"
//#include "cmd/bitmaphook.hpp"
//#include "cmd/filehook.hpp"
//#include "cmd/tmo.hpp"

#include "tools/traits.hpp"
#include "interface/types.hpp"
#include "interface/parameters.hpp"
#include "interface/hooks.hpp"
#include "interface/tagitem.hpp"
#include "interface/jpeg.hpp"

typedef JPEG JPG; // prevent naming collision
typedef FLOAT JFLOAT;

namespace MoxFiles
{

JPEGXTCodec::JPEGXTCodec(const Header &header, const ChannelList &channels) :
	VideoCodec(header, channels),
	_descriptor(header.frameRate(), header.width(), header.height(), MoxMxf::VideoDescriptor::VideoCodecJPEGXT)
{
	setWindows(_descriptor, header);
	
	const Channel *r_channel = channels.findChannel("R");
	const Channel *a_channel = channels.findChannel("A");
	
	if(r_channel == NULL)
		throw MoxMxf::ArgExc("Expected RGB(A) channels");
	
	
	_channels = (a_channel != NULL ? JPEGXT_RGBA : JPEGXT_RGB);
	
	
	const PixelType pixelType = r_channel->type;
	
	_depth = (pixelType == UINT8 ? JPEGXT_8 :
				pixelType == UINT10 ? JPEGXT_10 :
				pixelType == UINT12 ? JPEGXT_12 :
				pixelType == UINT16 ? JPEGXT_16 :
				pixelType == FLOAT ? JPEGXT_FLOAT :
				JPEGXT_8);
	
	const unsigned int bit_depth = (_depth == JPEGXT_8 ? 8 :
									_depth == JPEGXT_10 ? 10 :
									_depth == JPEGXT_12 ? 12 :
									_depth == JPEGXT_16 ? 16 :
									_depth == JPEGXT_FLOAT ? 254 :
									8);
	
	MoxMxf::RGBADescriptor::RGBALayout layout;
	
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('R', bit_depth));
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('G', bit_depth));
	layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('B', bit_depth));
	
	if(_channels == JPEGXT_RGBA)
		layout.push_back(MoxMxf::RGBADescriptor::RGBALayoutItem('A', bit_depth));
	
	_descriptor.setPixelLayout(layout);
	
	
	_lossless = isLossless(header);
	_quality = (_lossless ? 100 : getQuality(header));
}


JPEGXTCodec::JPEGXTCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) :
	VideoCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<const MoxMxf::RGBADescriptor &>(descriptor))
{
	assert(_descriptor.getVideoCodec() == MoxMxf::VideoDescriptor::VideoCodecJPEGXT);

	const MoxMxf::RGBADescriptor::RGBALayout &pixelLayout = _descriptor.getPixelLayout();
	
	for(MoxMxf::RGBADescriptor::RGBALayout::const_iterator i = pixelLayout.begin(); i != pixelLayout.end(); ++i)
	{
		const MoxMxf::RGBADescriptor::RGBALayoutItem &item = *i;
		
		if(item.code == 'A')
			_channels = JPEGXT_RGBA;
		
		if(item.depth == 8)
			_depth = JPEGXT_8;
		else if(item.depth == 10)
			_depth = JPEGXT_10;
		else if(item.depth == 12)
			_depth = JPEGXT_12;
		else if(item.depth == 16)
			_depth = JPEGXT_16;
		else if(item.depth == 254)
			_depth = JPEGXT_FLOAT;
		else
			throw MoxMxf::InputExc("Unexpected bit depth");
	}
	
	
	const PixelType pixel_type = (_depth == JPEGXT_8 ? UINT8 :
									_depth == JPEGXT_10 ? UINT10 :
									_depth == JPEGXT_12 ? UINT12 :
									_depth == JPEGXT_16 ? UINT16 :
									_depth == JPEGXT_FLOAT ? FLOAT :
									UINT8);
									
	channels.insert("R", Channel(pixel_type));
	channels.insert("G", Channel(pixel_type));
	channels.insert("B", Channel(pixel_type));
	
	if(_channels == JPEGXT_RGBA)
		channels.insert("A", Channel(pixel_type));
}


JPEGXTCodec::~JPEGXTCodec()
{

}


struct BitmapMemory {
  APTR         bmm_pMemPtr;     // interleaved memory for the HDR image
  APTR         bmm_pAlphaPtr;   // memory for the alpha channel
  ULONG        bmm_ulWidth;     // width in pixels.
  ULONG        bmm_ulHeight;    // height in pixels; this is only one block in our application.
  UWORD        bmm_usComponents;// number of rgb components.
  UBYTE        bmm_ucPixelType; // pixel type
};


static JPG_LONG BitmapHook(struct JPG_Hook *hook, struct JPG_TagItem *tags)
{
	struct BitmapMemory *bmm  = (struct BitmapMemory *)(hook->hk_pData);
	
	const JPG_LONG action = tags->GetTagData(JPGTAG_BIO_ACTION);
	
	if(action == JPGFLAG_BIO_REQUEST)
	{
		const UWORD comp = tags->GetTagData(JPGTAG_BIO_COMPONENT);
		const ULONG miny = tags->GetTagData(JPGTAG_BIO_MINY);
		const ULONG maxy = tags->GetTagData(JPGTAG_BIO_MAXY);
		
		assert(miny == 0);
		assert(maxy <= bmm->bmm_ulHeight);
		
		const UBYTE pixelType = bmm->bmm_ucPixelType;
		
		const size_t pixSize = (pixelType == CTYP_FLOAT ? sizeof(JFLOAT) :
								pixelType == CTYP_UWORD ? sizeof(WORD) :
								sizeof(UBYTE));
		
		assert(bmm->bmm_usComponents == 3);
		
		const size_t pixelSize = (pixSize * bmm->bmm_usComponents);
		
		const size_t rowBytes = (pixelSize * bmm->bmm_ulWidth);
		
		assert(bmm->bmm_pMemPtr != NULL);
		
		char *basePtr = (char *)bmm->bmm_pMemPtr;
		
		char *compPtr = (basePtr + (comp * pixSize));
		
		tags->SetTagPtr(JPGTAG_BIO_MEMORY, compPtr);
		tags->SetTagData(JPGTAG_BIO_WIDTH, bmm->bmm_ulWidth);
		tags->SetTagData(JPGTAG_BIO_HEIGHT, bmm->bmm_ulHeight);
		tags->SetTagData(JPGTAG_BIO_BYTESPERROW, rowBytes);
		tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL, pixelSize);
		tags->SetTagData(JPGTAG_BIO_PIXELTYPE, pixelType);
	}
	else
		assert(action == JPGFLAG_BIO_RELEASE); // only other one, not that I need it

	return 0;
}

static JPG_LONG AlphaHook(struct JPG_Hook *hook, struct JPG_TagItem *tags)
{
	struct BitmapMemory *bmm  = (struct BitmapMemory *)(hook->hk_pData);
	
	const JPG_LONG action = tags->GetTagData(JPGTAG_BIO_ACTION);
	
	if(action == JPGFLAG_BIO_REQUEST)
	{
		const ULONG miny = tags->GetTagData(JPGTAG_BIO_MINY);
		const ULONG maxy = tags->GetTagData(JPGTAG_BIO_MAXY);
		
		assert(miny == 0);
		assert(maxy <= bmm->bmm_ulHeight);
		
		const UBYTE pixelType = bmm->bmm_ucPixelType;
		
		const size_t pixSize = (pixelType == CTYP_FLOAT ? sizeof(JFLOAT) :
								pixelType == CTYP_UWORD ? sizeof(WORD) :
								sizeof(UBYTE));
		
		const size_t rowBytes = (pixSize * bmm->bmm_ulWidth);
		
		assert(bmm->bmm_pAlphaPtr != NULL);
		
		char *basePtr = (char *)bmm->bmm_pAlphaPtr;
		
		tags->SetTagPtr(JPGTAG_BIO_MEMORY, basePtr);
		tags->SetTagData(JPGTAG_BIO_WIDTH, bmm->bmm_ulWidth);
		tags->SetTagData(JPGTAG_BIO_HEIGHT, bmm->bmm_ulHeight);
		tags->SetTagData(JPGTAG_BIO_BYTESPERROW, rowBytes);
		tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL, pixSize);
		tags->SetTagData(JPGTAG_BIO_PIXELTYPE, pixelType);
	}
	else
		assert(action == JPGFLAG_BIO_RELEASE); // only other one, not that I need it

	return 0;
}

static JPG_LONG FileHook(struct JPG_Hook *hook, struct JPG_TagItem *tags)
{
	MemoryFile *file = (MemoryFile *)hook->hk_pData;
	
	const JPG_LONG action = tags->GetTagData(JPGTAG_FIO_ACTION);
	
	if(action == JPGFLAG_ACTION_READ || action == JPGFLAG_ACTION_WRITE)
	{
		UBYTE *buffer = (UBYTE *)tags->GetTagPtr(JPGTAG_FIO_BUFFER);
		ULONG size = tags->GetTagData(JPGTAG_FIO_SIZE);
		
		if(action == JPGFLAG_ACTION_READ)
		{
			return file->FileRead(buffer, size);
		}
		else
		{
			assert(action == JPGFLAG_ACTION_WRITE);
			
			return file->FileWrite(buffer, size);
		}
	}
	else if(action == JPGFLAG_ACTION_SEEK)
	{
		const LONG mode = tags->GetTagData(JPGTAG_FIO_SEEKMODE);
		const LONG offset = tags->GetTagData(JPGTAG_FIO_OFFSET);
		
		try
		{
			if(mode == JPGFLAG_OFFSET_END)
			{
				const LONG fileSize = file->FileSize();
				
				file->FileSeek(fileSize - offset);
			}
			else if(mode == JPGFLAG_OFFSET_CURRENT)
			{
				const LONG currentPos = file->FileTell();
				
				file->FileSeek(currentPos + offset);
			}
			else
			{
				assert(mode == JPGFLAG_OFFSET_BEGINNING);
				
				file->FileSeek(offset);
			}
			
			return 0;
		}
		catch(...)
		{
			return -1;
		}
	}
	else if(action == JPGFLAG_ACTION_QUERY)
	{
		return 0;
	}
	
	return -1;
}


void
JPEGXTCodec::compress(const FrameBuffer &frame)
{
	const Box2i dataW = dataWindow();
	
	const int width = (dataW.max.x - dataW.min.x + 1);
	const int height = (dataW.max.y - dataW.min.y + 1);
	
	
	const PixelType pixelType = (_depth == JPEGXT_8 ? UINT8 :
									_depth == JPEGXT_10 ? UINT10 :
									_depth == JPEGXT_12 ? UINT12 :
									_depth == JPEGXT_16 ? UINT16 :
									_depth == JPEGXT_FLOAT ? FLOAT :
									UINT8);
								
	const unsigned int bitDepth = PixelBits(pixelType);
	const size_t pixSize = PixelSize(pixelType);
	const bool isFloat = (pixelType == FLOAT);
	
	
	FrameBuffer tempFrameBuffer(dataW);
	
	DataChunkPtr rgbBuffer, alphaBuffer;
	
	const size_t rgbPixelSize = (3 * pixSize);
	const size_t rgbRowBytes = (rgbPixelSize * width);
	const size_t rgbBufferSize = (rgbRowBytes * height);
	
	rgbBuffer = new DataChunk(rgbBufferSize);
	
	assert(dataW.min.x == 0 && dataW.min.y == 0);
	
	char *rgbBufferPtr = (char *)rgbBuffer->Data;
	
	tempFrameBuffer.insert("R", Slice(pixelType, &rgbBufferPtr[0 * pixSize], rgbPixelSize, rgbRowBytes));
	tempFrameBuffer.insert("G", Slice(pixelType, &rgbBufferPtr[1 * pixSize], rgbPixelSize, rgbRowBytes));
	tempFrameBuffer.insert("B", Slice(pixelType, &rgbBufferPtr[2 * pixSize], rgbPixelSize, rgbRowBytes));
	
	
	const bool haveAlpha = (_channels == JPEGXT_RGBA);
	
	assert(!haveAlpha); // now working right now
	
	if(haveAlpha)
	{
		const size_t alphaPixelSize = pixSize;
		const size_t alphaRowBytes = (alphaPixelSize * width);
		const size_t alphaBufferSize = (alphaRowBytes * height);
		
		alphaBuffer = new DataChunk(alphaBufferSize);
		
		char *alphaBufferPtr = (char *)alphaBuffer->Data;
		
		assert(dataW.min.x == 0 && dataW.min.y == 0);
		
		tempFrameBuffer.insert("A", Slice(pixelType, alphaBufferPtr, alphaPixelSize, alphaRowBytes));
	}
	
	
	tempFrameBuffer.copyFromFrame(frame);
	
	
	const UBYTE jpgPixelType = (pixelType == UINT8 ? CTYP_UBYTE :
								pixelType == FLOAT ? CTYP_FLOAT :
								CTYP_UWORD);
	
	
	bool success = true;
	
	JPG *jpg = JPG::Construct(NULL);
	
	if(jpg == NULL)
		throw MoxMxf::NullExc("jpg is NULL");
	
	
	struct BitmapMemory bmm;
	
	bmm.bmm_pMemPtr = rgbBuffer->Data;
	bmm.bmm_pAlphaPtr = (alphaBuffer ? alphaBuffer->Data : NULL);
	bmm.bmm_ulWidth = width;
	bmm.bmm_ulHeight = height;
	bmm.bmm_usComponents = 3;
	bmm.bmm_ucPixelType = jpgPixelType;
	
	struct JPG_Hook bmhook(BitmapHook, &bmm);
	struct JPG_Hook alphahook(AlphaHook, &bmm);
	
	struct JPG_TagItem atags[] = {
		JPG_ValueTag(JPGTAG_IMAGE_PRECISION, bitDepth),
		JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT, isFloat),
		JPG_ValueTag(JPGTAG_IMAGE_QUALITY, _quality),
		//JPG_ValueTag(JPGTAG_RESIDUAL_QUALITY, _quality),
		JPG_ValueTag(JPGTAG_ALPHA_MODE, JPGFLAG_ALPHA_REGULAR),
		JPG_EndTag
	};
	
	struct JPG_TagItem tags[] = {
		JPG_PointerTag(JPGTAG_BIH_HOOK, &bmhook),
		JPG_PointerTag((haveAlpha ? JPGTAG_BIH_ALPHAHOOK : JPGTAG_TAG_IGNORE), &alphahook),
		//JPG_ValueTag(JPGTAG_ENCODER_LOOP_ON_INCOMPLETE, true),
		JPG_ValueTag(JPGTAG_IMAGE_WIDTH, width),
		JPG_ValueTag(JPGTAG_IMAGE_HEIGHT, height),
		JPG_ValueTag(JPGTAG_IMAGE_DEPTH, 3), // "depth" is number of components
		JPG_ValueTag(JPGTAG_IMAGE_PRECISION, bitDepth),
		JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT, isFloat),
		JPG_ValueTag(JPGTAG_IMAGE_QUALITY, _quality),
		JPG_ValueTag(JPGTAG_RESIDUAL_QUALITY, _quality), // HDR quality
		JPG_PointerTag((haveAlpha ? JPGTAG_ALPHA_TAGLIST : JPGTAG_TAG_IGNORE), atags),
		JPG_EndTag
	};
	
	
	const JPG_LONG imageProvided = jpg->ProvideImage(tags);
	
	if(imageProvided)
	{
		MemoryFile file;
	
		struct JPG_Hook filehook(FileHook, &file);
		
		struct JPG_TagItem iotags[] = {
			JPG_PointerTag(JPGTAG_HOOK_IOHOOK, &filehook),
			JPG_PointerTag(JPGTAG_HOOK_IOSTREAM, &file),
			JPG_EndTag
		};
		
		const JPG_LONG fileWritten = jpg->Write(iotags);
		
		if(fileWritten)
		{
			storeData(file.getDataChunk());
		}
		else
			success = false;
	}
	else
		success = false;
	
	
	JPG::Destruct(jpg);
	
	
	if(!success)
		throw MoxMxf::ArgExc("Error writing JPEG XT");
}


void
JPEGXTCodec::decompress(const DataChunk &data)
{
	bool success = true;

	JPG *jpg = JPG::Construct(NULL);
	
	if(jpg == NULL)
		throw MoxMxf::NullExc("jpg is NULL");
	
	
	MemoryFile file(data);
	
	struct JPG_Hook filehook(FileHook, &file);
	
	struct JPG_TagItem iotags[] = {
		JPG_PointerTag(JPGTAG_HOOK_IOHOOK, &filehook),
		JPG_PointerTag(JPGTAG_HOOK_IOSTREAM, &file),
		JPG_EndTag
	};
	
	
	const JPG_LONG didRead = jpg->Read(iotags);
	
	if(didRead)
	{
		struct JPG_TagItem atags[] = {
			JPG_ValueTag(JPGTAG_IMAGE_PRECISION, 0),
			JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT, false),
			JPG_ValueTag(JPGTAG_IMAGE_OUTPUT_CONVERSION, true),
			JPG_EndTag
		};
		
		struct JPG_TagItem itags[] = {
			JPG_ValueTag(JPGTAG_IMAGE_WIDTH, 0),
			JPG_ValueTag(JPGTAG_IMAGE_HEIGHT, 0),
			JPG_ValueTag(JPGTAG_IMAGE_DEPTH, 0),
			JPG_ValueTag(JPGTAG_IMAGE_PRECISION, 0),
			JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT, false),
			JPG_ValueTag(JPGTAG_IMAGE_OUTPUT_CONVERSION, true),
			JPG_ValueTag(JPGTAG_ALPHA_MODE, JPGFLAG_ALPHA_OPAQUE),
			JPG_PointerTag(JPGTAG_ALPHA_TAGLIST, atags),
			JPG_EndTag
		};
		
		const JPG_LONG didGetInfo = jpg->GetInformation(itags);
		
		if(didGetInfo)
		{
			const ULONG width = itags->GetTagData(JPGTAG_IMAGE_WIDTH);
			const ULONG height = itags->GetTagData(JPGTAG_IMAGE_HEIGHT);
			
			const Box2i dataW = dataWindow();
			
			assert(width == (dataW.max.x - dataW.min.x + 1));
			assert(height == (dataW.max.y - dataW.min.y + 1));
			assert(dataW.min.x == 0);
			assert(dataW.min.y == 0);
			
			
			const UBYTE rgbComponents = itags->GetTagData(JPGTAG_IMAGE_DEPTH); // depth is components
			
			assert(rgbComponents == 3);
			
			
			const UBYTE prec = itags->GetTagData(JPGTAG_IMAGE_PRECISION);
			const bool isFloat = !!itags->GetTagData(JPGTAG_IMAGE_IS_FLOAT);
			
			const PixelType mxfType = (_depth == JPEGXT_8 ? UINT8 :
										_depth == JPEGXT_10 ? UINT10 :
										_depth == JPEGXT_12 ? UINT12 :
										_depth == JPEGXT_16 ? UINT16 :
										_depth == JPEGXT_FLOAT ? FLOAT :
										UINT8);
										
			const PixelType pixelType = (isFloat ? FLOAT :
											prec == 8 ? UINT8 :
											prec == 10 ? UINT10 :
											prec == 12 ? UINT12 :
											prec == 16 ? UINT16 :
											UINT8);
			
			assert(pixelType == mxfType);
			
			const UBYTE jpgPixelType = (pixelType == UINT8 ? CTYP_UBYTE :
										pixelType == FLOAT ? CTYP_FLOAT :
										CTYP_UWORD);
			
			
			FrameBufferPtr frameBuffer = new FrameBuffer(dataW);
			
			
			DataChunkPtr rgbBuffer, alphaBuffer;
			
			const size_t rgbPixSize = PixelSize(pixelType);
			const size_t rgbPixelSize = (rgbComponents * rgbPixSize);
			const size_t rgbRowBytes = (width * rgbPixelSize);
			const size_t rgbBufferSize = (height * rgbRowBytes);
			
			rgbBuffer = new DataChunk(rgbBufferSize);
			
			char *rgbBufferPtr = (char *)rgbBuffer->Data;
			
			frameBuffer->insert("R", Slice(pixelType, &rgbBufferPtr[0 * rgbPixSize], rgbPixelSize, rgbRowBytes));
			frameBuffer->insert("G", Slice(pixelType, &rgbBufferPtr[1 * rgbPixSize], rgbPixelSize, rgbRowBytes));
			frameBuffer->insert("B", Slice(pixelType, &rgbBufferPtr[2 * rgbPixSize], rgbPixelSize, rgbRowBytes));
			
			frameBuffer->attachData(rgbBuffer);
			
			
			const bool convert = !!itags->GetTagData(JPGTAG_IMAGE_OUTPUT_CONVERSION);
			
			assert(!convert);

			
			const bool doAlpha = !!itags->GetTagData(JPGTAG_ALPHA_MODE);
			
			if(doAlpha)
			{
				const UBYTE aprec = atags->GetTagData(JPGTAG_IMAGE_PRECISION);
				const bool alphaFloat = !!atags->GetTagData(JPGTAG_IMAGE_IS_FLOAT);
				
				assert(aprec == prec);
				assert(alphaFloat == isFloat);
				
				const size_t alphaPixSize = PixelSize(pixelType);
				const size_t alphaRowBytes = (width * alphaPixSize);
				const size_t alphaBufferSize = (height * alphaRowBytes);
				
				alphaBuffer = new DataChunk(alphaBufferSize);
				
				char *alphaBufferPtr = (char *)alphaBuffer->Data;
				
				frameBuffer->insert("A", Slice(pixelType, alphaBufferPtr, alphaPixSize, alphaRowBytes));
				
				frameBuffer->attachData(alphaBuffer);
			}
			
			
			struct BitmapMemory bmm;
			
			bmm.bmm_pMemPtr = rgbBuffer->Data;
			bmm.bmm_pAlphaPtr = (alphaBuffer ? alphaBuffer->Data : NULL);
			bmm.bmm_ulWidth = width;
			bmm.bmm_ulHeight = height;
			bmm.bmm_usComponents = 3;
			bmm.bmm_ucPixelType = jpgPixelType;
			
			struct JPG_Hook bmhook(BitmapHook, &bmm);
			struct JPG_Hook alphahook(AlphaHook, &bmm);
			
			struct JPG_TagItem tags[] = {
				JPG_PointerTag(JPGTAG_BIH_HOOK, &bmhook),
				JPG_PointerTag(JPGTAG_BIH_ALPHAHOOK, &alphahook),
				JPG_EndTag
			};
			
			const JPG_LONG readFile = jpg->DisplayRectangle(tags);
			
			if(readFile)
			{
				storeFrame(frameBuffer);
			}
			else
				success = false;
		}
		else
			success = false;
	}
	else
		success = false;
	
	
	JPG::Destruct(jpg);
	
	
	if(!success)
		throw MoxMxf::ArgExc("Error reading JPEG XT");
}


bool
JPEGXTCodecInfo::canCompressType(PixelType pixelType) const
{
	return (pixelType == UINT8 || pixelType == UINT10 || pixelType == UINT12 || pixelType == UINT16 || pixelType == FLOAT);
}


ChannelCapabilities
JPEGXTCodecInfo::getChannelCapabilites() const
{
	return (Channels_RGB);
}


VideoCodec * 
JPEGXTCodecInfo::createCodec(const Header &header, const ChannelList &channels) const
{
	return new JPEGXTCodec(header, channels);
}

VideoCodec * 
JPEGXTCodecInfo::createCodec(const MoxMxf::VideoDescriptor &descriptor, Header &header, ChannelList &channels) const
{
	return new JPEGXTCodec(descriptor, header, channels);
}


} // namespace

