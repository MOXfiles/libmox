/*
 *  AES3Codec.cpp
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 5/11/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <MoxFiles/UncompressedPCMCodec.h>

#include <MoxFiles/Thread.h>

namespace MoxFiles
{

UncompressedPCMCodec::UncompressedPCMCodec(const Header &header, const AudioChannelList &channels) :
	AudioCodec(header, channels),
	_descriptor(header.frameRate(), header.sampleRate(), channels.size(), SampleBits(channels.type()))
{
	const UInt8 num_channels = channels.size();

	if(num_channels != 1 && num_channels != 2 && num_channels != 6)
		throw MoxMxf::LogicExc("Can't handle this channel config");
}


UncompressedPCMCodec::UncompressedPCMCodec(MoxMxf::AudioDescriptor *descriptor, Header &header, AudioChannelList &channels) :
	AudioCodec(descriptor, header, channels),
	_descriptor(dynamic_cast<MoxMxf::WaveAudioDescriptor &>(*descriptor))
{
	assert(header.sampleRate() == _descriptor.getAudioSamplingRate());

	const UInt32 bit_depth = _descriptor.getBitDepth();
	const UInt32 num_channels = _descriptor.getChannelCount();
	
	
	SampleType sampleType;
	
	switch(bit_depth)
	{
		case 8:
			sampleType = UNSIGNED8;
		break;
		
		case 16:
			sampleType = SIGNED16;
		break;

		case 24:
			sampleType = SIGNED24;
		break;

		case 32:
			sampleType = SIGNED32;
		break;
		
		default:
			throw MoxMxf::LogicExc("Not handling the provided bit depth");
	}
	
	
	std::vector<Name> channel_list = StandardAudioChannelList(num_channels);
	
	for(int i = 0; i < channel_list.size(); i++)
	{
		channels.insert(channel_list[i].text(), AudioChannel(sampleType));
	}
}


class CompressPCMTask : public Task
{
  public:
	CompressPCMTask(TaskGroup *group, char *dest_origin, ptrdiff_t dest_stride, UInt8 bit_depth, const AudioSlice &source_slice, UInt64 length);
	~CompressPCMTask() {}
	
	virtual void execute();

  private:
	char * const _dest_origin;
	const ptrdiff_t _dest_stride;
	const UInt8 _bit_depth;
	const AudioSlice &_source_slice;
	const UInt64 _length;
};

CompressPCMTask::CompressPCMTask(TaskGroup *group, char *dest_origin, ptrdiff_t dest_stride, UInt8 bit_depth, const AudioSlice &source_slice, UInt64 length) :
	Task(group),
	_dest_origin(dest_origin),
	_dest_stride(dest_stride),
	_bit_depth(bit_depth),
	_source_slice(source_slice),
	_length(length)
{

}

void
CompressPCMTask::execute()
{
	if(_bit_depth == 8)
	{
		assert(_source_slice.type == UNSIGNED8);
		
		UInt8 *out = (UInt8 *)_dest_origin;
		const UInt8 *in = (UInt8 *)_source_slice.base;
		
		const int out_step = _dest_stride / sizeof(UInt8);
		const int in_step = _source_slice.stride / sizeof(UInt8);
		
		for(int i = 0; i < _length; i++)
		{
			*out = *in;
			
			out += out_step;
			in += in_step;
		}
	}
	else if(_bit_depth == 16)
	{
		assert(_source_slice.type == SIGNED16);
		
		UInt8 *out = (UInt8 *)_dest_origin;
		const UInt16 *in = (UInt16 *)_source_slice.base;
		
		const int out_step = _dest_stride / sizeof(UInt8);
		const int in_step = _source_slice.stride / sizeof(UInt16);
		
		for(int i = 0; i < _length; i++)
		{
			out[0] = (*in & 0xff);
			out[1] = (*in & 0xff00) >> 8;
			
			out += out_step;
			in += in_step;
		}
	}
	else if(_bit_depth == 24)
	{
		assert(_source_slice.type == SIGNED24);
		
		UInt8 *out = (UInt8 *)_dest_origin;
		const Int32 *in = (Int32 *)_source_slice.base;
		
		const int out_step = _dest_stride / sizeof(UInt8);
		const int in_step = _source_slice.stride / sizeof(Int32);
		
		for(int i = 0; i < _length; i++)
		{
			const Int32 val = *in << 8; // convert 24-bit to 32-bit
			
			const UInt32 *uval = (const UInt32 *)&val;
			
			out[0] = (*uval & 0xff00) >> 8;
			out[1] = (*uval & 0xff0000) >> 16;
			out[2] = (*uval & 0xff000000) >> 24;
			
			out += out_step;
			in += in_step;
		}
	}
	else if(_bit_depth == 32)
	{
		assert(_source_slice.type == SIGNED32);
		
		UInt8 *out = (UInt8 *)_dest_origin;
		const UInt32 *in = (UInt32 *)_source_slice.base;
		
		const int out_step = _dest_stride / sizeof(UInt8);
		const int in_step = _source_slice.stride / sizeof(UInt32);
		
		for(int i = 0; i < _length; i++)
		{
			out[0] = (*in & 0xff) >> 0;
			out[1] = (*in & 0xff00) >> 8;
			out[2] = (*in & 0xff0000) >> 16;
			out[3] = (*in & 0xff000000) >> 24;
			
			out += out_step;
			in += in_step;
		}
	}
}


void
UncompressedPCMCodec::compress(const AudioBuffer &audio)
{
	const UInt32 bit_depth = _descriptor.getBitDepth();
	
	SampleType sampleType;
	
	switch(bit_depth)
	{
		case 8:
			sampleType = UNSIGNED8;
		break;
		
		case 16:
			sampleType = SIGNED16;
		break;

		case 24:
			sampleType = SIGNED24;
		break;

		case 32:
			sampleType = SIGNED32;
		break;
		
		default:
			throw MoxMxf::LogicExc("Bad bit depth");
	}
	
	bool input_matches = true;
	
	for(AudioBuffer::ConstIterator i = audio.begin(); i != audio.end(); ++i)
	{
		const AudioSlice &slice = i.slice();
		
		if(slice.type != sampleType)
			input_matches = false;
	}
	
	assert(input_matches); // currently converting as soon as the audio is pushed, so it should match here
	
	AudioBufferPtr converted_audio;
	
	if(!input_matches)
	{
		converted_audio = MakeAudioBufferType(audio, sampleType);
	}
	
	const AudioBuffer &audio_buf = (input_matches ? audio : *converted_audio);
	
	
	const UInt32 channels = _descriptor.getChannelCount();
	
	assert(channels == audio_buf.size());
	
	const UInt64 length = audio_buf.length();
	const UInt64 samples = length * channels;
	
	const Rational &sample_rate = _descriptor.getSampleRate();
	const Rational &audio_sampling_rate = _descriptor.getAudioSamplingRate();
	
	assert(samples == (channels * audio_sampling_rate.Numerator * sample_rate.Denominator) / (sample_rate.Numerator * audio_sampling_rate.Denominator));
	
	
	
	const size_t bytes_per_sample = (bit_depth + 7) / 8;
	const size_t stride = bytes_per_sample * channels;
	const size_t data_size = samples * bytes_per_sample;
	
	DataChunkPtr data = new DataChunk(data_size);
	
	
	std::vector<Name> channel_list = StandardAudioChannelList(channels);
	
	{
		TaskGroup taskGroup;
		
		char *data_origin = (char *)data->Data;
		
		for(int i = 0; i < channel_list.size(); i++)
		{	
			const char *name = channel_list[i].text();
			
			const AudioSlice *slice = audio_buf.findSlice(name);
			
			if(slice)
			{
				ThreadPool::addGlobalTask(new CompressPCMTask(&taskGroup, data_origin, stride, bit_depth, *slice, length));
			}
			else
				assert(false);
			
			data_origin += bytes_per_sample;
		}
	}
	
	storeData(data);
}


void
UncompressedPCMCodec::decompress(const DataChunk &data)
{
	const UInt32 bit_depth = _descriptor.getBitDepth();
	const size_t bytes_per_sample = (bit_depth + 7) / 8;
	
	assert(data.Size % bytes_per_sample == 0);
	
	const UInt64 samples = data.Size / bytes_per_sample;
	
	const UInt32 channels = _descriptor.getChannelCount();
	const Rational &sample_rate = _descriptor.getSampleRate();
	const Rational &audio_sampling_rate = _descriptor.getAudioSamplingRate();
	
	assert(samples == (channels * audio_sampling_rate.Numerator * sample_rate.Denominator) / (sample_rate.Numerator * audio_sampling_rate.Denominator));
	
	SampleType sampleType;
	size_t decoded_sample_size;
	
	switch(bit_depth)
	{
		case 8:
			sampleType = UNSIGNED8;
			decoded_sample_size = sizeof(unsigned char);
		break;
		
		case 16:
			sampleType = SIGNED16;
			decoded_sample_size = sizeof(short);
		break;

		case 24:
			sampleType = SIGNED24;
			decoded_sample_size = sizeof(int);
		break;

		case 32:
			sampleType = SIGNED32;
			decoded_sample_size = sizeof(int);
		break;
		
		default:
			throw MoxMxf::LogicExc("Not handling the provided bit depth");
	}
	
	
	DataChunkPtr buf_data = new DataChunk(decoded_sample_size * samples);
	
	char *buf_origin = (char *)buf_data->Data;
	
	const UInt64 length = samples / channels;
	
	const ptrdiff_t stride = channels * decoded_sample_size;
	
	
	AudioBufferPtr buf = new AudioBuffer(length);
	
	std::vector<Name> channel_list = StandardAudioChannelList(channels);
	
	for(int i = 0; i < channel_list.size(); i++)
	{
		buf->insert(channel_list[i].text(), AudioSlice(sampleType, buf_origin + (decoded_sample_size * i), stride));
	}
		
	buf->attachData(buf_data);
	
	
	if(bit_depth == 8)
	{
		UInt8 *in = (UInt8 *)data.Data;
		
		UInt8 *out = (UInt8 *)buf_origin;
		
		for(int i = 0; i < samples; i++)
		{
			*out++ = *in++;
		}
	}
	else if(bit_depth == 16)
	{
		UInt8 *in = (UInt8 *)data.Data;
		
		Int16 *out = (Int16 *)buf_origin;
		
		for(int i = 0; i < samples; i++)
		{
			// fill (unsigned) 16-bit space with 16 bits
			const UInt16 uval =	(in[0] << 0) |
								(in[1] << 8);
			
			// cast as signed 32-bit
			const Int16 *val = (Int16 *)&uval;
			
			// put back down to 24-bit values
			*out = *val;
			
			in += 2;
			out++;
		}
	}
	else if(bit_depth == 24)
	{
		UInt8 *in = (UInt8 *)data.Data;
		
		Int32 *out = (Int32 *)buf_origin;
		
		for(int i = 0; i < samples; i++)
		{
			// fill (unsigned) 32-bit space with 24 bits
			const UInt32 uval =	(in[0] << 8) |   // *in++, *in++, *in++ does not work for some reason
								(in[1] << 16) |
								(in[2] << 24);
			
			// cast as signed 32-bit
			const Int32 *val = (Int32 *)&uval;
			
			// put back down to 24-bit values
			*out = (*val >> 8);
			
			in += 3;
			out++;
		}
	}
	else if(bit_depth == 32)
	{
		UInt8 *in = (UInt8 *)data.Data;
		
		Int32 *out = (Int32 *)buf_origin;
		
		for(int i = 0; i < samples; i++)
		{
			// fill (unsigned) 32-bit space with 24 bits
			const UInt32 uval =	(in[0] << 0) |
								(in[1] << 8) |
								(in[2] << 16) |
								(in[3] << 24);
			
			// cast as signed 32-bit
			const Int32 *val = (Int32 *)&uval;
			
			// put back down to 24-bit values
			*out = *val;
			
			in += 4;
			out++;
		}
	}
	else
		throw MoxMxf::NoImplExc("Not handling this bit depth");
	
	
	storeBuffer(buf);
}


AudioChannelCapabilities
UncompressedPCMCodecInfo::getChannelCapabilites() const
{
	return AudioChannels_All;
}

AudioCodec *
UncompressedPCMCodecInfo::createCodec(const Header &header, const AudioChannelList &channels) const
{
	return new UncompressedPCMCodec(header, channels);
}

AudioCodec *
UncompressedPCMCodecInfo::createCodec(MoxMxf::AudioDescriptor *descriptor, Header &header, AudioChannelList &channels) const
{
	return new UncompressedPCMCodec(descriptor, header, channels);
}

} // namespace


