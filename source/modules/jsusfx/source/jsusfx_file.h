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

#pragma once

#include "WDL/eel2/ns-eel.h"
#include <iostream>
#include <vector>

class JsusFx;
struct JsusFxFileAPI;
struct JsusFxFileAPI_Basic;
struct JsusFx_File;
struct RIFFSoundData;

struct JsusFxFileAPI {
	virtual ~JsusFxFileAPI() { }
	void init(NSEEL_VMCTX vm);
	
	virtual int file_open(JsusFx & jsusFx, const char * filename) { return -1; }
	virtual bool file_close(JsusFx & jsusFx, const int handle) { return false; }
	virtual int file_avail(JsusFx & jsusFx, const int handle) { return 0; }
	virtual bool file_riff(JsusFx & jsusFx, const int handle, int & numChannels, int & sampleRate) { return false; }
	virtual bool file_text(JsusFx & jsusFx, const int handle) { return false; }
	virtual int file_mem(JsusFx & jsusFx, const int handle, EEL_F * result, const int numValues) { return 0; }
	virtual bool file_var(JsusFx & jsusFx, const int handle, EEL_F & result) { return false; }
};

/*
JsusFxFileAPI_Basic provides a basic File API implementation which uses JsusFx_File for reading and the JsusFxPathLibrary to resolve and open files
*/

struct JsusFxFileAPI_Basic : JsusFxFileAPI
{
	static const int kMaxFileHandles = 16;

	JsusFx_File * files[kMaxFileHandles];
	
	JsusFxFileAPI_Basic();
	
	virtual int file_open(JsusFx & jsusFx, const char * filename) override;
	virtual bool file_close(JsusFx & jsusFx, const int index) override;
	virtual int file_avail(JsusFx & jsusFx, const int index) override;
	virtual bool file_riff(JsusFx & jsusFx, const int index, int & numChannels, int & sampleRate) override;
	virtual bool file_text(JsusFx & jsusFx, const int index) override;
	virtual int file_mem(JsusFx & jsusFx, const int index, EEL_F * dest, const int numValues) override;
	virtual bool file_var(JsusFx & jsusFx, const int index, EEL_F & dest) override;
};

/*
JsusFx_File is used to implement the File API. it provides functionality to read RIFF sound data, text files and raw data files according to how these functions should behave according to Reaper's documentation. documentation for the exact operation of these functions isn't available, so a best effort attempt has been made to implement them

// example usage loading a sound file:

JsusFx_File file;
if (file.open(jsusFx, "sound.wav"))
{
	int numChannels;
 	int sampleRate;
 
	if (file.riff(numChannels, sampleRate))
	{
		const int numSamples = file.avail();
 
		EEL_F * samples = new EEL_F[numSamples];
 
		if (file.mem(numSamples, samples))
		{
			printf("successfully loaded some sound samples!\n");
		}
	}
 
	file.close();
}

// example usage loading values from a text file:

JsusFx_File file;
if (file.open(jsusFx, "kernelValues.txt"))
{
	if (file.text())
	{
		while (file.avail())
		{
			EEL_F value;
 
			if (file.var(value))
			{
				printf("%.2f\n", value);
			}
		}
	}
 
	file.close();
}
*/

struct JsusFx_File
{
	enum Mode
	{
		kMode_None,
		kMode_Text,
		kMode_Sound
	};
	
	std::istream * stream;
	
	Mode mode;
	
	RIFFSoundData * soundData;
	
	int readPosition;
	
	std::vector<EEL_F> vars;
	
	JsusFx_File();
	~JsusFx_File();
	
	bool open(JsusFx & jsusFx, const char * _filename);
	void close(JsusFx & jsusFx);
	
	bool riff(int & numChannels, int & sampleRate);
	bool text();
	
	int avail() const;
	
	bool mem(const int numValues, EEL_F * dest);
	bool var(EEL_F & value);
};
