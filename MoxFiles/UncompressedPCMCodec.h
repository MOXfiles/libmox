/*
 *  AES3Codec.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 5/11/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_AES3CODEC_H
#define MOXFILES_AES3CODEC_H

#include <MoxFiles/Codec.h>

namespace MoxFiles
{

	class UncompressedPCMCodec : public AudioCodec
	{
	  public:
		UncompressedPCMCodec(const Header &header, const AudioChannelList &channels);
		UncompressedPCMCodec(MoxMxf::AudioDescriptor *descriptor, Header &header, AudioChannelList &channels);
		virtual ~UncompressedPCMCodec() {}
		
		virtual MoxMxf::AudioDescriptor * getDescriptor() { return &_descriptor; }
		
		virtual void compress(const AudioBuffer &audio);
		virtual void decompress(const DataChunk &data);

	  private:
		MoxMxf::WaveAudioDescriptor _descriptor;
	};
	
	
	class UncompressedPCMCodecInfo : public AudioCodecInfo
	{
	  public:
		UncompressedPCMCodecInfo() {}
		virtual ~UncompressedPCMCodecInfo() {}
		
		virtual AudioChannelCapabilities getChannelCapabilites() const;
		
		virtual AudioCodec * createCodec(const Header &header, const AudioChannelList &channels) const; // compression
		virtual AudioCodec * createCodec(MoxMxf::AudioDescriptor *descriptor, Header &header, AudioChannelList &channels) const; // decompression
	};

} // namespace

#endif // MOXFILES_AES3CODEC_H