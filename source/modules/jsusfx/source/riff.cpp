/*
	Copyright (C) 2018 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "riff.h"
#include <ctype.h>
#include <stdint.h>
#include <string.h>

//

#ifndef nullptr
	#define nullptr NULL
#endif

//

template <typename T>
bool read(const uint8_t *& src, int & srcSize, T & result)
{
	if (srcSize < sizeof(T))
		return false;
	
	result = *(T*)src;
	
	src += sizeof(T);
	srcSize -= sizeof(T);
	
	return true;
}

bool read(const uint8_t *& src, int & srcSize, void * result, const int numBytes)
{
	if (srcSize < numBytes)
		return false;
	
	memcpy(result, src, numBytes);
	
	src += numBytes;
	srcSize -= numBytes;
	
	return true;
}

bool skip(const uint8_t *& src, int & srcSize, const int numBytes)
{
	if (srcSize < numBytes)
		return false;
	
	src += numBytes;
	srcSize -= numBytes;
	
	return true;
}

static void logError(const char * format, ...)
{

}

//

enum Chunk
{
	kChunk_RIFF,
	kChunk_WAVE,
	kChunk_FMT,
	kChunk_DATA,
	kChunk_OTHER
};

static bool checkId(const char * id, const char * match)
{
	for (int i = 0; i < 4; ++i)
		if (tolower(id[i]) != tolower(match[i]))
			return false;
	
	return true;
}

static bool readChunk(const uint8_t *& src, int & srcSize, Chunk & chunk, int32_t & size)
{
	char id[4];
	
	if (!read(src, srcSize, id, 4))
		return false;
	
	//logDebug("RIFF chunk: %c%c%c%c", id[0], id[1], id[2], id[3]);
	
	chunk = kChunk_OTHER;
	size = 0;
	
	if (checkId(id, "WAVE"))
	{
		chunk = kChunk_WAVE;
		return true;
	}
	else if (checkId(id, "fmt "))
	{
		chunk = kChunk_FMT;
		return true;
	}
	else
	{
		if (checkId(id, "RIFF"))
			chunk = kChunk_RIFF;
		else if (checkId(id, "data"))
			chunk = kChunk_DATA;
		else if (checkId(id, "LIST") || checkId(id, "FLLR") || checkId(id, "JUNK") || checkId(id, "bext"))
			chunk = kChunk_OTHER;
		else
		{
			logError("unknown RIFF chunk: %c%c%c%c", id[0], id[1], id[2], id[3]);
			return false; // unknown
		}
		
		if (!read(src, srcSize, size))
			return false;
		
		if (size < 0)
			return false;
		
		return true;
	}
}

//

RIFFSoundData::RIFFSoundData()
{
	memset(this, 0, sizeof(RIFFSoundData));
}

RIFFSoundData::~RIFFSoundData()
{
	if (sampleData != 0)
	{
		delete [] (char*)sampleData;
		sampleData = 0;
	}
}

RIFFSoundData * loadRIFF(const void * _src, const int _srcSize)
{
	const uint8_t * src = (uint8_t*)_src;
	int srcSize = _srcSize;
	
	bool hasFmt = false;
	int32_t fmtLength;
	int16_t fmtCompressionType; // format code is a better name. 1 = PCM/integer, 2 = ADPCM, 3 = float, 7 = u-law
	int16_t fmtChannelCount;
	int32_t fmtSampleRate;
	int32_t fmtByteRate;
	int16_t fmtBlockAlign;
	int16_t fmtBitDepth;
	int16_t fmtExtraLength;
	
	uint8_t * bytes = nullptr;
	int numBytes = 0;
	
	bool done = false;
	
	do
	{
		Chunk chunk;
		int32_t byteCount;
		
		if (!readChunk(src, srcSize, chunk, byteCount))
			return 0;
		
		if (chunk == kChunk_RIFF || chunk == kChunk_WAVE)
		{
			// just process sub chunks
		}
		else if (chunk == kChunk_FMT)
		{
			bool ok = true;
			
			ok &= read(src, srcSize, fmtLength);
			ok &= read(src, srcSize, fmtCompressionType);
			ok &= read(src, srcSize, fmtChannelCount);
			ok &= read(src, srcSize, fmtSampleRate);
			ok &= read(src, srcSize, fmtByteRate);
			ok &= read(src, srcSize, fmtBlockAlign);
			ok &= read(src, srcSize, fmtBitDepth);
			if (fmtCompressionType != 1)
				ok &= read(src, srcSize, fmtExtraLength);
			else
				fmtExtraLength = 0;
			
			if (!ok)
			{
				logError("failed to read FMT chunk");
				return 0;
			}
			
			if (fmtCompressionType != 1)
			{
				logError("only PCM is supported. type: %d", fmtCompressionType);
				ok = false;
			}
			if (fmtChannelCount <= 0)
			{
				logError("invalid channel count: %d", fmtChannelCount);
				ok = false;
			}
			if (fmtBitDepth != 8 && fmtBitDepth != 16 && fmtBitDepth != 24 && fmtBitDepth != 32)
			{
				logError("bit depth not supported: %d", fmtBitDepth);
				ok = false;
			}
			
			if (!ok)
				return 0;
			
			hasFmt = true;
		}
		else if (chunk == kChunk_DATA)
		{
			if (hasFmt == false)
				return 0;
			
			bytes = new uint8_t[byteCount];
			
			if (!read(src, srcSize, bytes, byteCount))
			{
				logError("failed to load WAVE data");
				delete [] bytes;
				return 0;
			}
			
			// convert data if necessary
		
			if (fmtBitDepth == 8)
			{
				// for 8 bit data the integers are unsigned. convert them to signed here
				
				const uint8_t * srcValues = bytes;
				int8_t * dstValues = (int8_t*)bytes;
				const int numValues = byteCount;
				
				for (int i = 0; i < numValues; ++i)
				{
					const int8_t value = int8_t(int(srcValues[i]) - 128);
					
					dstValues[i] = value;
				}
			}
			else if (fmtBitDepth == 24)
			{
				const int sampleCount = byteCount / 3;
				float * samplesData = new float[sampleCount];
				
				for (int i = 0; i < sampleCount; ++i)
				{
					int32_t value = (bytes[i * 3 + 0] << 8) | (bytes[i * 3 + 1] << 16) | (bytes[i * 3 + 2] << 24);
					
					value >>= 8;
					
					samplesData[i] = value / float(1 << 23);
				}
				
				delete[] bytes;
				bytes = nullptr;
				
				bytes = (uint8_t*)samplesData;
				
				fmtBitDepth = 32;
				byteCount = byteCount * 4 / 3;
			}
			else if (fmtBitDepth == 32)
			{
				const int32_t * srcValues = (int32_t*)bytes;
				float * dstValues = (float*)bytes;
				const int numValues = byteCount / 4;
				
				for (int i = 0; i < numValues; ++i)
				{
					dstValues[i] = float(srcValues[i] / double(1 << 31));
				}
			}
			
			numBytes = byteCount;
			
			done = true;
		}
		else if (chunk == kChunk_OTHER)
		{
			//logDebug("wave loader: skipping %d bytes of list chunk", size);
			
			skip(src, srcSize, byteCount);
		}
	}
	while (!done);
	
	if (false)
	{
		// suppress unused variables warnings
		fmtLength = 0;
		fmtByteRate = 0;
		fmtBlockAlign = 0;
		fmtExtraLength = 0;
	}
	
	RIFFSoundData * soundData = new RIFFSoundData();
	soundData->channelSize = fmtBitDepth / 8;
	soundData->channelCount = fmtChannelCount;
	soundData->sampleCount = numBytes / (fmtBitDepth / 8 * fmtChannelCount);
	soundData->sampleRate = fmtSampleRate;
	soundData->sampleData = bytes;
	
	return soundData;
}
