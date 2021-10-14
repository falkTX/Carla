/*
 * Copyright 2018 Pascal Gauthier, Marcel Smit
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * *distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "jsusfx.h"
#include "jsusfx_file.h"
#include "jsusfx_serialize.h"
#include "riff.h"
#include <assert.h>

#define EEL_FILE_GET_INTERFACE(opaque) ((opaque) ? (((JsusFx*)opaque)->fileAPI) : nullptr)

#define REAPER_GET_INTERFACE(opaque) (*(JsusFx*)opaque)

static EEL_F NSEEL_CGEN_CALL _file_open(void *opaque, EEL_F *_index)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
	const int index = *_index;
	
	WDL_FastString * fs = nullptr;
	const char * filename = jsusFx.getString(index, &fs);
	
	if (filename == nullptr)
		return -1;
	
	const int handle = fileAPI->file_open(jsusFx, filename);
	
	// file handle zero is used as the special serialization file
	assert(handle != 0);
	
	return handle;
}

static EEL_F NSEEL_CGEN_CALL _file_close(void *opaque, EEL_F *_handle)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	// file handle zero is used as the special serialization file
	assert(handle != 0);
	
	if (fileAPI->file_close(jsusFx, handle))
		return 0;
	else
		return -1;
}

static EEL_F NSEEL_CGEN_CALL _file_avail(void *opaque, EEL_F *_handle)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
  	const int handle = *_handle;
	
	if (handle == 0)
	{
		if (jsusFx.serializer == nullptr)
			return 0;
		else
			return jsusFx.serializer->file_avail();
	}
	else
	{
  		return fileAPI->file_avail(jsusFx, handle);
	}
}

static EEL_F NSEEL_CGEN_CALL _file_riff(void *opaque, EEL_F *_handle, EEL_F *_numChannels, EEL_F *_sampleRate)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	// file handle zero is used as the special serialization file
	assert(handle != 0);
	
	int numChannels;
	int sampleRate;
	
	if (fileAPI->file_riff(jsusFx, handle, numChannels, sampleRate) == false)
	{
		*_numChannels = 0;
		*_sampleRate = 0;
		return -1;
	}
	
	*_numChannels = numChannels;
	*_sampleRate = sampleRate;
	
	return 0;
}

static EEL_F NSEEL_CGEN_CALL _file_text(void *opaque, EEL_F *_handle)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	// file handle zero is used as the special serialization file
	assert(handle != 0);
	
	if (fileAPI->file_text(jsusFx, handle) == false)
		return -1;
	
	return 1;
}

static EEL_F NSEEL_CGEN_CALL _file_mem(void *opaque, EEL_F *_handle, EEL_F *_destOffset, EEL_F *_numValues)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	const int destOffset = (int)(*_destOffset + 0.001);
	
	EEL_F * dest = NSEEL_VM_getramptr(jsusFx.m_vm, destOffset, nullptr);
	
	if (dest == nullptr)
		return 0;
	
	const int numValues = (int)*_numValues;
	
	if (handle == 0)
	{
		if (jsusFx.serializer == nullptr)
			return 0;
		else
			return jsusFx.serializer->file_mem(dest, numValues);
	}
	else
	{
		return fileAPI->file_mem(jsusFx, handle, dest, numValues);
	}
}

static EEL_F NSEEL_CGEN_CALL _file_var(void *opaque, EEL_F *_handle, EEL_F *dest)
{
	JsusFxFileAPI *fileAPI = EEL_FILE_GET_INTERFACE(opaque);
	JsusFx &jsusFx = REAPER_GET_INTERFACE(opaque);
	
	const int handle = *_handle;
	
	if (handle == 0)
	{
		if (jsusFx.serializer == nullptr)
			return 0;
		else
			return jsusFx.serializer->file_var(*dest);
	}
	else
	{
		if (fileAPI->file_var(jsusFx, handle, *dest) == false)
			return 0;
		else
			return 1;
	}
}

void JsusFxFileAPI::init(NSEEL_VMCTX vm)
{
	NSEEL_addfunc_retval("file_open",1,NSEEL_PProc_THIS,&_file_open);
	NSEEL_addfunc_retval("file_close",1,NSEEL_PProc_THIS,&_file_close);
	NSEEL_addfunc_retval("file_avail",1,NSEEL_PProc_THIS,&_file_avail);
	NSEEL_addfunc_retval("file_riff",3,NSEEL_PProc_THIS,&_file_riff);
	NSEEL_addfunc_retval("file_text",1,NSEEL_PProc_THIS,&_file_text);
	NSEEL_addfunc_retval("file_mem",3,NSEEL_PProc_THIS,&_file_mem);
	NSEEL_addfunc_retval("file_var",2,NSEEL_PProc_THIS,&_file_var);
}

//

JsusFxFileAPI_Basic::JsusFxFileAPI_Basic()
{
	memset(files, 0, sizeof(files));
}

int JsusFxFileAPI_Basic::file_open(JsusFx & jsusFx, const char * filename)
{
	std::string resolvedPath;
	
	if (jsusFx.pathLibrary.resolveDataPath(filename, resolvedPath) == false)
	{
		jsusFx.displayError("failed to resolve data path");
		return -1;
	}
	
	for (int i = 1; i < kMaxFileHandles; ++i)
	{
		if (files[i] == nullptr)
		{
			files[i] = new JsusFx_File();
			
			if (files[i]->open(jsusFx, resolvedPath.c_str()) == false)
			{
				jsusFx.displayError("failed to open file: %s", resolvedPath.c_str());
				
				delete files[i];
				files[i] = nullptr;
				
				return -1;
			}
			else
			{
				return i;
			}
		}
	}
	
	jsusFx.displayError("failed to find a free file handle");
	return -1;
}

bool JsusFxFileAPI_Basic::file_close(JsusFx & jsusFx, const int index)
{
	if (index < 0 || index >= kMaxFileHandles)
	{
		jsusFx.displayError("invalid file handle");
		return false;
	}
	
	if (files[index] == nullptr)
	{
		jsusFx.displayError("file not opened");
		return false;
	}
	
	files[index]->close(jsusFx);
	
	delete files[index];
	files[index] = nullptr;
	
	return true;
}

int JsusFxFileAPI_Basic::file_avail(JsusFx & jsusFx, const int index)
{
	if (index < 0 || index >= kMaxFileHandles)
	{
		jsusFx.displayError("invalid file handle");
		return 0;
	}
	
	if (files[index] == nullptr)
	{
		jsusFx.displayError("file not opened");
		return 0;
	}
	
	return files[index]->avail();
}

bool JsusFxFileAPI_Basic::file_riff(JsusFx & jsusFx, const int index, int & numChannels, int & sampleRate)
{
	if (index < 0 || index >= kMaxFileHandles)
	{
		jsusFx.displayError("invalid file handle");
		return false;
	}
	
	if (files[index] == nullptr)
	{
		jsusFx.displayError("file not opened");
		return false;
	}
	
	if (files[index]->riff(numChannels, sampleRate) == false)
	{
		jsusFx.displayError("failed to parse RIFF");
		return false;
	}
	
	return true;
}

bool JsusFxFileAPI_Basic::file_text(JsusFx & jsusFx, const int index)
{
	if (index < 0 || index >= kMaxFileHandles)
	{
		jsusFx.displayError("invalid file handle");
		return false;
	}
	
	if (files[index] == nullptr)
	{
		jsusFx.displayError("file not opened");
		return false;
	}
	
	if (files[index]->text() == false)
	{
		jsusFx.displayError("failed to parse text");
		return false;
	}
	
	return true;
}

int JsusFxFileAPI_Basic::file_mem(JsusFx & jsusFx, const int index, EEL_F * dest, const int numValues)
{
	if (index < 0 || index >= kMaxFileHandles)
	{
		jsusFx.displayError("invalid file handle");
		return 0;
	}
	
	if (files[index] == nullptr)
	{
		jsusFx.displayError("file not opened");
		return 0;
	}
	
	if (files[index]->mem(numValues, dest) == false)
	{
		jsusFx.displayError("failed to read data");
		return 0;
	}
	else
		return numValues;
}

bool JsusFxFileAPI_Basic::file_var(JsusFx & jsusFx, const int index, EEL_F & dest)
{
	if (index < 0 || index >= kMaxFileHandles)
	{
		jsusFx.displayError("invalid file handle");
		return 0;
	}
	
	if (files[index] == nullptr)
	{
		jsusFx.displayError("file not opened");
		return 0;
	}
	
	if (files[index]->var(dest) == false)
	{
		jsusFx.displayError("failed to read value");
		return 0;
	}
	else
		return 1;
}

//

JsusFx_File::JsusFx_File()
	: stream(nullptr)
	, mode(kMode_None)
	, soundData(nullptr)
	, readPosition(0)
	, vars()
{
}

JsusFx_File::~JsusFx_File()
{
	assert(stream == nullptr);
}

bool JsusFx_File::open(JsusFx & jsusFx, const char * filename)
{
	// open the stream
	
	stream = jsusFx.pathLibrary.open(filename);
	
	if (stream == nullptr)
	{
		jsusFx.displayError("failed to open file: %s", filename);
		return false;
	}
	else
	{
		return true;
	}
}

void JsusFx_File::close(JsusFx & jsusFx)
{
	vars.clear();
	
	delete soundData;
	soundData = nullptr;
	
	jsusFx.pathLibrary.close(stream);
}

bool JsusFx_File::riff(int & numChannels, int & sampleRate)
{
	assert(mode == kMode_None);
	
	numChannels = 0;
	sampleRate = 0;
	
	// reset read state
	
	mode = kMode_None;
	readPosition = 0;
	
	// load RIFF file
	
	bool success = true;
	
	success &= stream != nullptr;
	
	int size = 0;;
	
	if (success)
	{
		try
		{
			stream->seekg(0, std::ios_base::end);
			success &= stream->fail() == false;
			
			size = stream->tellg();
			success &= size != -1;
			
			stream->seekg(0, std::ios_base::beg);
			success &= stream->fail() == false;
		}
		catch (std::exception & e)
		{
			(void)e;
			success &= false;
		}
	}
	
	success &= size > 0;
	
	uint8_t * bytes = nullptr;
	
	if (success)
	{
		bytes = new uint8_t[size];
	
		try
		{
			stream->read((char*)bytes, size);
			
			success &= stream->fail() == false;
		}
		catch (std::exception & e)
		{
			(void)e;
			success &= false;
		}
	}
	
	if (success)
	{
		soundData = loadRIFF(bytes, size);
	}
	
	if (bytes != nullptr)
	{
		delete [] bytes;
		bytes = nullptr;
	}
	
	if (soundData == nullptr || (soundData->channelSize != 2 && soundData->channelSize != 4))
	{
		if (soundData != nullptr)
		{
			delete soundData;
			soundData = nullptr;
		}
		
		return false;
	}
	else
	{
		mode = kMode_Sound;
		numChannels = soundData->channelCount;
		sampleRate = soundData->sampleRate;
		return true;
	}
}

bool JsusFx_File::text()
{
	assert(mode == kMode_None);
	
	// reset read state
	
	mode = kMode_None;
	readPosition = 0;
	
	// load text file
	
	try
	{
		if (stream == nullptr)
		{
			return false;
		}
		
		while (!stream->eof())
		{
			char line[2048];
			
			stream->getline(line, sizeof(line), '\n');
			
			// a poor way of skipping comments. assume / is the start of // and strip anything that come after it
			char * pos = strchr(line, '/');
			if (pos != nullptr)
				*pos = 0;
			
			const char * p = line;
			
			for (;;)
			{
				// skip trailing white space
				
				while (*p && isspace(*p))
					p++;
				
				// reached end of the line ?
				
				if (*p == 0)
					break;
				
				// parse the value
				
				double var;
				
				if (sscanf(p, "%lf", &var) == 1)
					vars.push_back(var);
				
				// skip the value
				
				while (*p && !isspace(*p))
					p++;
			}
		}
		
		mode = kMode_Text;
		
		return true;
	}
	catch (std::exception & e)
	{
		(void)e;
		return false;
	}
}

int JsusFx_File::avail() const
{
	if (mode == kMode_None)
		return 0;
	else if (mode == kMode_Text)
		return readPosition == vars.size() ? 0 : 1;
	else if (mode == kMode_Sound)
		return soundData->sampleCount * soundData->channelCount - readPosition;
	else
		return 0;
}

bool JsusFx_File::mem(const int numValues, EEL_F * dest)
{
	if (mode == kMode_None)
		return false;
	else if (mode == kMode_Text)
		return false;
	else if (mode == kMode_Sound)
	{
		if (numValues > avail())
			return false;
		for (int i = 0; i < numValues; ++i)
		{
			if (soundData->channelSize == 2)
			{
				const short * values = (short*)soundData->sampleData;
				
				dest[i] = values[readPosition] / float(1 << 15);
			}
			else if (soundData->channelSize == 4)
			{
				const float * values = (float*)soundData->sampleData;
				
				dest[i] = values[readPosition];
			}
			else
			{
				assert(false);
			}
			
			readPosition++;
		}
		
		return true;
	}
	else
		return false;
}

bool JsusFx_File::var(EEL_F & value)
{
	if (mode == kMode_None)
		return false;
	else if (mode == kMode_Text)
	{
		if (avail() < 1)
			return false;
		value = vars[readPosition];
		readPosition++;
		return true;
	}
	else if (mode == kMode_Sound)
		return false;
	else
		return false;
}
