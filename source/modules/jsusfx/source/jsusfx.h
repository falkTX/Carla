/*
 * Copyright 2014-2015 Pascal Gauthier
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

#include <string.h>
#include <iostream>
#include <string>
#include <vector>

#include "WDL/eel2/ns-eel.h"
#include "WDL/eel2/ns-eel-int.h"

#ifndef nullptr
	#define nullptr NULL
#endif

class eel_string_context_state;

class JsusFx;
struct JsusFxFileAPI;
struct JsusFxGfx;
struct JsusFxPathLibrary;
struct JsusFxSerializer;

struct JsusFx_FileInfo;
class JsusFx_Slider;
struct JsusFx_Sections;

class WDL_FastString;

//

class JsusFx_Slider {
public:
	static const int kMaxName = 63;
	
    float def, min, max, inc;

	char name[kMaxName + 1];
    char desc[64];
    EEL_F *owner;
    bool exists;
	
    std::vector<std::string> enumNames;
    bool isEnum;

    JsusFx_Slider() {
        def = min = max = inc = 0;
        name[0] = 0;
        desc[0] = 0;
        exists = false;
        owner = nullptr;
        isEnum = false;
    }

    bool config(JsusFx &fx, const int index, const char *param, const int lnumber);
    
    /**
     * Return true if the value has changed
     */
    bool setValue(float v) {
    	if ( min < max ) {
			if ( v < min ) {
				v = min;
			} else if ( v > max ) {
				v = max;
			}
		} else {
			if ( v < max ) {
				v = max;
			} else if ( v > min ) {
				v = min;
			}
		}
        if ( v == *owner )
            return false;
        *owner = v;
        return true;
    }
	
    float getValue() const {
    	return *owner;
	}
};

struct JsusFx_FileInfo {
	std::string filename;
	
	bool isValid() const {
		return !filename.empty();
	}
	
	bool init(const char *_filename) {
		filename = _filename;
		
		return isValid();
	}
};

struct JsusFxPathLibrary {
	virtual ~JsusFxPathLibrary() {
	}
	
	virtual bool resolveImportPath(const std::string &importPath, const std::string &parentPath, std::string &resolvedPath) {
		return false;
	}
	
	virtual bool resolveDataPath(const std::string &importPath, std::string &resolvedPath) {
		return false;
	}
	
	virtual std::istream* open(const std::string &path) {
		return nullptr;
	}
	
	virtual void close(std::istream *&stream) {
	}
};

struct JsusFxPathLibrary_Basic : JsusFxPathLibrary {
	std::string dataRoot;
	
	std::vector<std::string> searchPaths;
	
	JsusFxPathLibrary_Basic(const char * _dataRoot);
	
	void addSearchPath(const std::string & path);
	
	static bool fileExists(const std::string &filename);

	virtual bool resolveImportPath(const std::string &importPath, const std::string &parentPath, std::string &resolvedPath) override;
	virtual bool resolveDataPath(const std::string &importPath, std::string &resolvedPath) override;
	
	virtual std::istream* open(const std::string &path) override;
	virtual void close(std::istream *&stream) override;
};

class JsusFx {
protected:
    NSEEL_CODEHANDLE codeInit, codeSlider, codeBlock, codeSample, codeGfx, codeSerialize;

    bool computeSlider;
    void releaseCode();
    bool compileSection(int state, const char *code, int line_offset);
    bool processImport(JsusFxPathLibrary &pathLibrary, const std::string &currentPath, const std::string &importPath, JsusFx_Sections &sections, const int compileFlags);
    bool readHeader(JsusFxPathLibrary &pathLibrary, const std::string &currentPath, std::istream &input);
    bool readSections(JsusFxPathLibrary &pathLibrary, const std::string &currentPath, std::istream &input, JsusFx_Sections &sections, const int compileFlags);
    bool compileSections(JsusFx_Sections &sections, const int compileFlags);

public:
	static const int kMaxSamples = 64;
	// Theoretically, it is 64 slider, but starting @ 1, we are simply allowing slider0 to exists 
	static const int kMaxSliders = 65;
	static const int kMaxFileInfos = 128;
	
	enum CompileFlags
	{
		kCompileFlag_CompileGraphicsSection = 1 << 0,
		kCompileFlag_CompileSerializeSection = 1 << 1
	};
	
	enum PlaybackState // see 'play_state' at https://www.reaper.fm/sdk/js/vars.php#js_specialvars
	{
		kPlaybackState_Error = 0,
		kPlaybackState_Playing = 1,
		kPlaybackState_Paused = 2,
		kPlaybackState_Recording = 5,
		kPlaybackState_RecordingPaused = 6
	};
	
    NSEEL_VMCTX m_vm;
    JsusFx_Slider sliders[kMaxSliders];
    char desc[64];
    
    EEL_F *tempo, *play_state, *play_position, *beat_position, *ts_num, *ts_denom;
    EEL_F *ext_noinit, *ext_nodenorm, *pdc_delay, *pdc_bot_cd, *pdc_top_ch;
    EEL_F *srate, *num_ch, *samplesblock;
    EEL_F *spl[kMaxSamples], *trigger;
    EEL_F *ext_midi_bus, *midi_bus;
    EEL_F dummyValue;
	
    int numInputs;
    int numOutputs;
    int numValidInputChannels;
	
	JsusFxPathLibrary &pathLibrary;
	
    JsusFxFileAPI *fileAPI;
    JsusFx_FileInfo fileInfos[kMaxFileInfos];
	
	// midi receive buffer. pointer is incremented and the size decremented by the appropriate number of bytes whenever midirecv is called from within the script
	uint8_t *midi;
    int midiSize;
	
	// midi send buffer. the pointer and capacity stay the same. size is incremented by the number of bytes written by midisend or midisend_buf
    uint8_t * midiSendBuffer;
    int midiSendBufferCapacity;
    int midiSendBufferSize;
	
    JsusFxGfx *gfx;
    int gfx_w;
    int gfx_h;
	
	JsusFxSerializer *serializer;

    JsusFx(JsusFxPathLibrary &pathLibrary);
    virtual ~JsusFx();

    bool compile(JsusFxPathLibrary &pathLibrary, const std::string &path, const int compileFlags);
    bool readHeader(JsusFxPathLibrary &pathLibrary, const std::string &path);
    void prepare(int sampleRate, int blockSize);
    
    // move slider, normalizeSlider is used to normalize the value to a constant (0 means no normalization)
    void moveSlider(int idx, float value, int normalizeSlider = 0);
	
    void setMidi(const void * midi, int numBytes);
    void setMidiSendBuffer(void * buffer, int maxBytes);
	
    void setTransportValues(
    	const double tempo,
    	const PlaybackState playbackState,
    	const double playbackPositionInSeconds,
    	const double beatPosition,
    	const int timeSignatureNumerator,
    	const int timeSignatureDenumerator);
	
    bool process(const float **input, float **output, int size, int numInputChannels, int numOutputChannels);
    bool process64(const double **input, double **output, int size, int numInputChannels, int numOutputChannels);
    void draw();
    bool serialize(JsusFxSerializer & serializer, const bool write);
	
    const char * getString(int index, WDL_FastString ** fs);
	
	bool hasGraphicsSection() const { return codeGfx != nullptr; }
	bool hasSerializeSection() const { return codeSerialize != nullptr; }
	
    bool handleFile(int index, const char *filename);
	
    virtual void displayMsg(const char *fmt, ...) = 0;
    virtual void displayError(const char *fmt, ...) = 0;

    void dumpvars();
    
    static void init();
    
    // ==============================================================
    eel_string_context_state *m_string_context;
};
