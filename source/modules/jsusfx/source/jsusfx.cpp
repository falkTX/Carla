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

#include "jsusfx.h"
#include "jsusfx_file.h"
#include "jsusfx_gfx.h"
#include "jsusfx_serialize.h"

#include <string.h>
#ifndef WIN32
	#include <unistd.h>
#endif

#include "WDL/ptrlist.h"
#include "WDL/assocarray.h"
#include "WDL/mutex.h"

#define REAPER_GET_INTERFACE(opaque) ((opaque) ? ((JsusFx*)opaque) : nullptr)

#define AUTOVAR(name) name = NSEEL_VM_regvar(m_vm, #name); *name = 0
#define AUTOVARV(name,value) name = NSEEL_VM_regvar(m_vm, #name); *name = value

#define EEL_STRING_GET_CONTEXT_POINTER(opaque) (((JsusFx *)opaque)->m_string_context)
#ifndef EEL_STRING_STDOUT_WRITE
  #ifndef EELSCRIPT_NO_STDIO
    #define EEL_STRING_STDOUT_WRITE(x,len) { fwrite(x,len,1,stdout); fflush(stdout); }
  #endif
#endif

static WDL_Mutex atomic_mutex;
#define EEL_ATOMIC_SET_SCOPE(opaque) WDL_Mutex *mutex = (opaque ? ((JsusFx*)opaque)->m_atomic_mutex : &atomic_mutex);
#define EEL_ATOMIC_ENTER mutex->Enter()
#define EEL_ATOMIC_LEAVE mutex->Leave()

#include "WDL/eel2/eel_strings.h"
#include "WDL/eel2/eel_misc.h"
#include "WDL/eel2/eel_fft.h"
#include "WDL/eel2/eel_mdct.h"
#include "WDL/eel2/eel_atomic.h"

#include <fstream> // to check if files exist
#include <cassert>

#ifdef _WIN32
#  include <windows.h>
static std::wstring toWideString(const std::string &u8str);
#endif

// Reaper API

static EEL_F NSEEL_CGEN_CALL _reaper_sliderchange(void *opaque, INT_PTR np, EEL_F **parms)
{
  JsusFx *ctx = REAPER_GET_INTERFACE(opaque);

  assert(np == 1);
  (void)np;

  uint64_t changebits = 0;

  // does this refer to a slider variable?
  JsusFx::VarSliderAssoc::iterator it = ctx->var_slider_assoc.find(parms[0]);

  if (it != ctx->var_slider_assoc.end()) { // parameter is a slider
    int slider = it->second; //NOTE(jpc) see notes about 1-indexing
    if (slider > 0 && slider < JsusFx::kMaxSliders)
      changebits = (uint64_t)1 << (it->second - 1);
  }
  else { // parameter is a bitmask
    changebits = (uint64_t)*parms[0];
  }

  ctx->sliderchangebits |= changebits;

  return 0;
}

static EEL_F * NSEEL_CGEN_CALL _reaper_slider(void *opaque, EEL_F *n)
{
  JsusFx *ctx = REAPER_GET_INTERFACE(opaque);
  const int index = *n;
  if (index >= 0 && index < ctx->kMaxSliders)
  	return ctx->sliders[index].owner;
  else {
    ctx->dummyValue = 0;
	return &ctx->dummyValue;
  }
}

static EEL_F * NSEEL_CGEN_CALL _reaper_spl(void *opaque, EEL_F *n)
{
  JsusFx *ctx = REAPER_GET_INTERFACE(opaque);
  const int index = *n;
  if (index >= 0 && index < ctx->numValidInputChannels)
  	return ctx->spl[index];
  else {
    ctx->dummyValue = 0;
	return &ctx->dummyValue;
  }
}

static EEL_F NSEEL_CGEN_CALL _midirecv(void *opaque, INT_PTR np, EEL_F **parms)
{
	JsusFx *ctx = REAPER_GET_INTERFACE(opaque);
	if (ctx->midiInputReadPos >= ctx->midiInput.size())
		return 0;

	JsusFx::EventHeader event;
	const uint8_t *data = nullptr;

	while (!data && ctx->midiInputReadPos < ctx->midiInput.size()) {
		data = &ctx->midiInput[ctx->midiInputReadPos];
		memcpy(&event, data, sizeof(event));
		data += sizeof(event);
		ctx->midiInputReadPos += sizeof(event) + event.length;
		// pass through the sysex events
		if (event.length > 3) {
			ctx->addOutputEvent(event.offset, data, event.length);
			data = nullptr;
		}
	}

	if (!data)
		return 0;

	uint8_t msg1 = 0;
	uint8_t msg2 = 0;
	uint8_t msg3 = 0;

	switch (event.length)
	{
		case 3:
			msg3 = data[2];
			// fall through
		case 2:
			msg2 = data[1];
			// fall through
		case 1:
			msg1 = data[0];
			break;
	}

 	*parms[0] = event.offset;
 	*parms[1] = msg1;
	if (np >= 4) {
		*parms[2] = msg2;
		*parms[3] = msg3;
	} else {
		*parms[2] = msg2 + msg3 * 256;
	}

	return 1;
}

static EEL_F NSEEL_CGEN_CALL _midirecv_buf(void *opaque, INT_PTR np, EEL_F **parms)
{
	JsusFx *ctx = REAPER_GET_INTERFACE(opaque);
	if (ctx->midiInputReadPos >= ctx->midiInput.size())
		return 0;

	int buf = (int)*parms[1];
	int recvlen = (int)*parms[2];

	int dcap = 0;
	EEL_F *dest = NSEEL_VM_getramptr(ctx->m_vm, buf, &dcap);
	if (!dest)
		return 0;

	// limit the received length to buffer capacity, preventing overflow
	if (recvlen > dcap)
		recvlen = dcap;

	///
	JsusFx::EventHeader event;
	const uint8_t *data = nullptr;

	while (!data && ctx->midiInputReadPos < ctx->midiInput.size()) {
		data = &ctx->midiInput[ctx->midiInputReadPos];
		memcpy(&event, data, sizeof(event));
		data += sizeof(event);
		ctx->midiInputReadPos += sizeof(event) + event.length;
		// pass through the events which are too large for the buffer
		if (event.length > recvlen) {
			ctx->addOutputEvent(event.offset, data, event.length);
			data = nullptr;
		}
	}

	if (!data)
		return 0;

    ///
	*parms[0] = event.offset;
	for (int i = 0; i < event.length; ++i)
		dest[i] = data[i];
	return event.length;
}

// determine the length of a midi message according to its status byte
// if length is dynamic, returns 0
unsigned midi_sizeof(uint8_t id)
{
	if ((id >> 7) == 0) {
		return 0;
	}
	else if ((id >> 4) != 0b1111) {
		static const uint8_t sizetable[8] = {
			3, 3, 3, 3, 2, 2, 3 };
		return sizetable[(id >> 4) & 0b111];
	}
	else {
		static const uint8_t sizetable[16] = {
			0, 2, 3, 2, 1, 1, 1, 0,
			1, 1, 1, 1, 1, 1, 1, 1 };
		return sizetable[id & 0b1111];
	}
}

static EEL_F NSEEL_CGEN_CALL _midisend(void *opaque, INT_PTR np, EEL_F **parms)
{
	JsusFx *ctx = REAPER_GET_INTERFACE(opaque);
	
	int offset;
	uint8_t msg1;
	uint8_t msg2;
	uint8_t msg3;
	
	if (np == 3) {
		offset = (int)*parms[0];
		msg1 = (uint8_t)*parms[1];
		const unsigned msg23 = (unsigned)*parms[2];
		msg2 = (uint8_t)(msg23 & 0xff);
		msg3 = (uint8_t)(msg23 >> 8);
	} else if (np == 4) {
		offset = (int)*parms[0];
		msg1 = (uint8_t)*parms[1];
		msg2 = (uint8_t)*parms[2];
		msg3 = (uint8_t)*parms[3];
	} else {
		return 0;
	}
	
	const uint8_t data[] = {msg1, msg2, msg3};

	// NOTE(jpc) correct the length of the message
	// in case it should be less than 3 bytes
	int length = midi_sizeof(msg1);
	if (length == 0) // don't know what message this is
		length = 3;

	if (!ctx->addOutputEvent(offset, data, length))
		return 0;
	
	return msg1;
}

static EEL_F NSEEL_CGEN_CALL _midisend_buf(void *opaque, INT_PTR np, EEL_F **parms)
{
	JsusFx *ctx = REAPER_GET_INTERFACE(opaque);
	if (np == 3) {
		const int offset = (int)*parms[0];
		const int buf = (int)*parms[1];
		const int len = (int)*parms[2];

		if (len <= 0)
			return 0;

		int scap = 0;
		const EEL_F *src = NSEEL_VM_getramptr(ctx->m_vm, buf, &scap);
		if (scap < len)
			return 0;

		uint8_t bytes[NSEEL_RAM_ITEMSPERBLOCK]; // 64 kB
		for (int i = 0; i < len; ++i)
			bytes[i] = (uint8_t)src[i];

		if (!ctx->addOutputEvent(offset, bytes, len))
			return 0;
		return len;
	} else {
		return 0;
	}
}

// todo : remove __stub
static EEL_F NSEEL_CGEN_CALL __stub(void *opaque, INT_PTR np, EEL_F **parms)
{
  return 0.0;
}

//

struct JsusFx_Section {
	WDL_String code;
	int lineOffset;
	
	JsusFx_Section()
	{
		lineOffset = 0;
	}
};

struct JsusFx_Sections {
	JsusFx_Section init;
	JsusFx_Section slider;
	JsusFx_Section block;
	JsusFx_Section sample;
	JsusFx_Section gfx;
	JsusFx_Section serialize;
};

//

static const char *skipWhite(const char *text) {
	while ( *text && isspace(*text) )
		text++;
	
	return text;
}

static const char *nextToken(const char *text) {
	while ( *text && *text != ',' && *text != '=' && *text != '<' && *text != '>' && *text != '{' && *text != '}' )
		text++;

	return text;
}

bool JsusFx_Slider::config(JsusFx &fx, const int index, const char *param, const int lnumber) {
	char buffer[2048];
	strncpy(buffer, param, 2048);
	buffer[sizeof(buffer)-1] = '\0';
	
	def = min = max = inc = 0;
	exists = false;
	
	enumNames.clear();
	isEnum = false;
	
	bool hasName = false;
	
	const char *tmp = nextToken(buffer);
	for (const char *next; *(next = nextToken(tmp)); ) {
		tmp = (tmp == next) ? (tmp + 1) : next;
	}
	strncpy(desc, tmp, 64);
	desc[63] = '\0';

	tmp = buffer;
	
	if ( isalpha(*tmp) ) {
		// extended syntax of format "slider1:variable_name=5<0,10,1>slider description"
		const char *begin = tmp;
		while ( *tmp && *tmp != '=' )
			tmp++;
		if ( *tmp != '=' ) {
			fx.displayError("Expected '=' at end of slider name %d", lnumber);
			return false;
		}
		const char *end = tmp;
		int len = end - begin;
		if ( len > JsusFx_Slider::kMaxName ) {
			fx.displayError("Slider name too long %d", lnumber);
			return false;
		}
		for ( int i = 0; i < len; ++i )
			name[i] = begin[i];
		name[len] = 0;
		hasName = true;
		tmp++;
	}
	
	if ( !sscanf(tmp, "%f", &def) )
		return false;
	
	tmp = nextToken(tmp);
	
	if ( *tmp != '<' )
	{
		//NOTE(jpc) slider without <> section, handle it permissively
		/*fx.displayError("slider info is missing");
		return false;*/
	}
	else
	{
		tmp++;
		
		if ( !sscanf(tmp, "%f", &min) )
		{
			fx.displayError("failed to read min value");
			return false;
		}
	
		tmp = nextToken(tmp);
		
		if ( *tmp != ',' )
		{
			fx.displayError("max value is missing");
			return false;
		}
		else
		{
			tmp++;
			
			if ( !sscanf(tmp, "%f", &max) )
			{
				fx.displayError("failed to read max value");
				return false;
			}
			
			tmp = nextToken(tmp);
			
			if ( *tmp == ',')
			{
				tmp++;
				
				tmp = skipWhite(tmp);
				
				if ( !sscanf(tmp, "%f", &inc) )
				{
					//log("failed to read increment value");
					//return false;
					
					inc = 0;
				}
				
				tmp = nextToken(tmp);
				
				if ( *tmp == '{' )
				{
					isEnum = true;
					
					inc = 1;
					
					tmp++;
					
					while ( true )
					{
						const char *end = nextToken(tmp);
						
						const std::string name(tmp, end);
						
						enumNames.push_back(name);
						
						tmp = end;
						
						if ( *tmp == 0 )
						{
							fx.displayError("enum value list not properly terminated");
							return false;
						}
						
						if ( *tmp == '}' )
						{
							tmp++;
							break;
						}
						// closing '}' is not required, so be permissive here
						if ( *tmp == '>' )
						{
							break;
						}

						tmp++;
					}
				}
			}
		}
	}
	
	if (hasName == false) {
		sprintf(name, "slider%d", index);
	}
	
	owner = NSEEL_VM_regvar(fx.m_vm, name);
	
	*owner = def;
	exists = true;
	return true;
}

//

JsusFxPathLibrary_Basic::JsusFxPathLibrary_Basic(const char * _dataRoot) {
	if ( _dataRoot != nullptr )
		dataRoot = _dataRoot;
}

void JsusFxPathLibrary_Basic::addSearchPath(const std::string & path) {
	if ( path.empty() )
		return;
	
	// make sure it ends with '/' or '\\'
	
	if ( path.back() == '/' || path.back() == '\\' )
		searchPaths.push_back(path);
	else
		searchPaths.push_back(path + "/");
}

bool JsusFxPathLibrary_Basic::fileExists(const std::string &filename) {
#ifdef _WIN32
	std::ifstream is(toWideString(filename).c_str());
#else
	std::ifstream is(filename);
#endif
	return is.is_open();
}

bool JsusFxPathLibrary_Basic::resolveImportPath(const std::string &importPath, const std::string &parentPath, std::string &resolvedPath) {
	const size_t pos = parentPath.rfind('/');
	if ( pos != std::string::npos )
		resolvedPath = parentPath.substr(0, pos + 1);
	
	if ( fileExists(resolvedPath + importPath) ) {
		resolvedPath = resolvedPath + importPath;
		return true;
	}
	
	for ( std::string & searchPath : searchPaths ) {
		if ( fileExists(resolvedPath + searchPath + importPath) ) {
			resolvedPath = resolvedPath + searchPath + importPath;
			return true;
		}
	}
	
	return false;
}

bool JsusFxPathLibrary_Basic::resolveDataPath(const std::string &importPath, std::string &resolvedPath) {
	if ( !dataRoot.empty() )
		resolvedPath = dataRoot + "/" + importPath;
	else
		resolvedPath = importPath;
	return fileExists(resolvedPath);
}

std::istream* JsusFxPathLibrary_Basic::open(const std::string &path) {
#ifdef _WIN32
	std::ifstream *stream = new std::ifstream(toWideString(path).c_str());
#else
	std::ifstream *stream = new std::ifstream(path);
#endif
	if ( stream->is_open() == false ) {
		delete stream;
		stream = nullptr;
	}
	
	return stream;
}

void JsusFxPathLibrary_Basic::close(std::istream *&stream) {
	delete stream;
	stream = nullptr;
}

//

JsusFx::JsusFx(JsusFxPathLibrary &_pathLibrary)
	: pathLibrary(_pathLibrary) {
    m_vm = NSEEL_VM_alloc();
    codeInit = codeSlider = codeBlock = codeSample = codeGfx = codeSerialize = NULL;
    NSEEL_VM_SetCustomFuncThis(m_vm,this);

    m_string_context = new eel_string_context_state();
    m_atomic_mutex = new WDL_Mutex;
    eel_string_initvm(m_vm);
    computeSlider = false;
    sliderchangebits = 0;
    srate = 0;
	
    pathLibrary = _pathLibrary;
	
    fileAPI = nullptr;
		
    midiInput.reserve(8192);
    midiOutput.reserve(8192);
    midiInputReadPos = 0;
	
	gfx = nullptr;
    gfx_w = 0;
    gfx_h = 0;
		
    serializer = nullptr;
	
    for (int i = 0; i < kMaxSamples; ++i) {
    	char name[16];
    	sprintf(name, "spl%d", i);
		
    	spl[i] = NSEEL_VM_regvar(m_vm, name);
    	*spl[i] = 0;
	}
	
	numInputs = 0;
	numOutputs = 0;
    inputNames.reserve(16);
    outputNames.reserve(16);
	
	numValidInputChannels = 0;
	
    AUTOVAR(srate);
    AUTOVARV(num_ch, 2);
    AUTOVAR(samplesblock);
		
    AUTOVAR(trigger);
		
	// transport state. use setTransportValues to set these
    AUTOVARV(tempo, 120); // playback tempo in beats per minute
    AUTOVARV(play_state, 1); // playback state. see the PlaybackState enum for details
    AUTOVAR(play_position); // current playback position in seconds
    AUTOVAR(beat_position); // current playback position in beats (beats = quarternotes in /4 time signatures)
    AUTOVARV(ts_num, 0); // time signature nominator. i.e. 3 if using 3/4 time
    AUTOVARV(ts_denom, 4); // time signature denominator. i.e. 4 if using 3/4 time
		
    AUTOVAR(ext_noinit);
    AUTOVAR(ext_nodenorm); // set to 1 to disable noise added to signals to avoid denormals from popping up
	
	// midi bus support
    AUTOVAR(ext_midi_bus); // when set to 1, support for midi buses is enabled. otherwise, only bus 0 is active and others will pass through
    AUTOVAR(midi_bus);
	
	// Reaper API
	NSEEL_addfunc_varparm("slider_automate",1,NSEEL_PProc_THIS,&__stub); // todo : implement slider_automate. add Reaper api interface?
	NSEEL_addfunc_varparm("slider_next_chg",2,NSEEL_PProc_THIS,&__stub); // todo : implement slider_next_chg. add Reaper api interface?
	NSEEL_addfunc_exparms("sliderchange",1,NSEEL_PProc_THIS,&_reaper_sliderchange);
	NSEEL_addfunc_retptr("slider",1,NSEEL_PProc_THIS,&_reaper_slider);
	NSEEL_addfunc_retptr("spl",1,NSEEL_PProc_THIS,&_reaper_spl);
	NSEEL_addfunc_varparm("midirecv",3,NSEEL_PProc_THIS,&_midirecv);
	NSEEL_addfunc_varparm("midirecv_buf",3,NSEEL_PProc_THIS,&_midirecv_buf);
    NSEEL_addfunc_varparm("midisend",3,NSEEL_PProc_THIS,&_midisend);
    NSEEL_addfunc_varparm("midisend_buf",3,NSEEL_PProc_THIS,&_midisend_buf);
}

JsusFx::~JsusFx() {
    releaseCode();
    if (m_vm) 
        NSEEL_VM_free(m_vm);
    delete m_string_context;
    delete m_atomic_mutex;
}

bool JsusFx::compileSection(int state, const char *code, int line_offset) {
    if ( code[0] == 0 )
        return true;

    char errorMsg[4096];

	//printf("section code:\n");
	//printf("%s", code);

    switch(state) {
    case 0:
        codeInit = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeInit == NULL ) {
            snprintf(errorMsg, 4096, "@init line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            return false;
        }
        break;
    case 1:
        codeSlider = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeSlider == NULL ) {
            snprintf(errorMsg, 4096, "@slider line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            return false;
        }
        break;
    case 2: 
        codeBlock = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeBlock == NULL ) {
            snprintf(errorMsg, 4096, "@block line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            return false;
        }
        break;
    case 3:
        codeSample = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeSample == NULL ) {
            snprintf(errorMsg, 4096, "@sample line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            return false;
        }
        break;
    case 4:
        codeGfx = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeGfx == NULL ) {
            snprintf(errorMsg, 4096, "@gfx line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            return false;
        }
        break;
    case 5:
        codeSerialize = NSEEL_code_compile_ex(m_vm, code, line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if ( codeSerialize == NULL ) {
            snprintf(errorMsg, 4096, "@serialize line %s", NSEEL_code_getcodeerror(m_vm));
            displayError(errorMsg);
            return false;
        }
        break;
    default:
        //printf("unknown block");
        break;
    }

    m_string_context->update_named_vars(m_vm);
    return true;
}

bool JsusFx::processImport(JsusFxPathLibrary &pathLibrary, const std::string &path, const std::string &importPath, JsusFx_Sections &sections, const int compileFlags) {
	bool result = true;
	
	//displayMsg("Importing %s", path.c_str());
	
	std::string resolvedPath;
	if ( ! pathLibrary.resolveImportPath(importPath, path, resolvedPath) ) {
		displayError("Failed to resolve import file path %s", importPath.c_str());
		return false;
	}

	std::istream *is = pathLibrary.open(resolvedPath);

	if ( is != nullptr ) {
		result &= readSections(pathLibrary, resolvedPath, *is, sections, compileFlags);
	} else {
		displayError("Failed to open imported file %s", importPath.c_str());
		result &= false;
	}
	
	pathLibrary.close(is);
	
	return result;
}

static char *trim(char *line, bool trimStart, bool trimEnd)
{
	if (trimStart) {
		while (*line && isspace(*line))
			line++;
	}
	
	if (trimEnd) {
		char *last = line;
		while (last[0] && last[1])
			last++;
		for (char *b = last; isspace(*b) && b >= line; b--)
			*b = 0;
	}
	
	return line;
}

bool JsusFx::readHeader(JsusFxPathLibrary &pathLibrary, const std::string &path, std::istream &input) {
	char line[4096];
	
	inputNames.clear();
	outputNames.clear();
	
    for(int lnumber = 1; ! input.eof(); lnumber++) {
		input.getline(line, sizeof(line), '\n');
		
        if ( line[0] == '@' )
            break;
		
		if ( ! strnicmp(line, "slider", 6) ) {
			int target = 0;
			if ( ! sscanf(line, "slider%d:", &target) )
				continue;
			if ( target < 0 || target >= kMaxSliders )
				continue;
			
			JsusFx_Slider &slider = sliders[target];
			
			char *p = line+7;
			while ( *p && *p != ':' )
				p++;
			if ( *p != ':' )
				continue;
			p++;
			
			if ( ! slider.config(*this, target, p, lnumber) ) {
				displayError("Incomplete slider @line %d (%s)", lnumber, line);
				return false;
			}
			trim(slider.desc, false, true);
			var_slider_assoc[slider.owner] = target;
			continue;
		}
		else if ( ! strncmp(line, "desc:", 5) ) {
			char *src = line+5;
			src = trim(src, true, true);
			strncpy(desc, src, 64);
			continue;
		}
		else if ( ! strncmp(line, "in_pin:", 7) ) {
			char *src = line+7;
			src = trim(src, true, true);
            if ( ! strncmp(src, "none", 4) ) {
                inputNames.clear();
                numInputs = -1;
            } else {
                if ( numInputs != -1 ) {
                    inputNames.push_back(src);
                    numInputs++;
                }
            }
		}
		else if ( ! strncmp(line, "out_pin:", 8) ) {
			char *src = line+8;
			src = trim(src, true, true);
            if ( ! strncmp(src, "none", 4) ) {
                outputNames.clear();
                numOutputs = -1;
            } else {
                if ( numOutputs != -1 ) {
                    outputNames.push_back(src);
                    numOutputs++;
                }
            }
		}
    }
	
	return true;
}

bool JsusFx::readSections(JsusFxPathLibrary &pathLibrary, const std::string &path, std::istream &input, JsusFx_Sections &sections, const int compileFlags) {
    WDL_String * code = nullptr;
    char line[4096];
	
	inputNames.clear();
	outputNames.clear();
	
	// are we reading the header or sections?
	bool isHeader = true;
	
    for(int lnumber = 1; ! input.eof(); lnumber++) {
		input.getline(line, sizeof(line), '\n');
		const int l = input.gcount();
		
        if ( line[0] == '@' ) {
            char *b = line + 1;
			
			b = trim(b, false, true);
			
			// we've begun reading sections now
			isHeader = false;
			
			JsusFx_Section *section = nullptr;
            if ( ! strnicmp(b, "init", 4) )
                section = &sections.init;
            else if ( ! strnicmp(b, "slider", 6) )
                section = &sections.slider;
            else if ( ! strnicmp(b, "block", 5) )
                section = &sections.block;
            else if ( ! strnicmp(b, "sample", 6) )
                section = &sections.sample;
            else if ( ! strnicmp(b, "gfx", 3) && (compileFlags & kCompileFlag_CompileGraphicsSection) != 0 ) {
            	if ( sscanf(b+3, "%d %d", &gfx_w, &gfx_h) != 2 ) {
            		gfx_w = 0;
            		gfx_h = 0;
				}
                section = &sections.gfx;
			}
			else if ( ! strnicmp(b, "serialize", 9) && (compileFlags & kCompileFlag_CompileSerializeSection) != 0  )
                section = &sections.serialize;
			
            if ( section != nullptr ) {
				code = &section->code;
				section->lineOffset = lnumber;
			} else {
				code = nullptr;
			}
			
            continue;
        }
        
        if ( code != nullptr ) {
            //int l = strlen(line);
			
            if ( l > 0 && line[l-1] == '\r' )
                line[l-1] = 0;
            
            if ( line[0] != 0 ) {
                code->Append(line);
            }
            code->Append("\n");
            continue;
        }

        if (isHeader) {
            if ( ! strnicmp(line, "slider", 6) ) {
                int target = 0;
                if ( ! sscanf(line, "slider%d:", &target) )
                    continue;
                if ( target < 0 || target >= kMaxSliders )
                    continue;
				
				JsusFx_Slider &slider = sliders[target];
				
                char *p = line+7;
                while ( *p && *p != ':' )
                	p++;
                if ( *p != ':' )
                	continue;
				p++;
					
                if ( ! slider.config(*this, target, p, lnumber) ) {
                    displayError("Incomplete slider @line %d (%s)", lnumber, line);
                    return false;
                }
                trim(slider.desc, false, true);
                var_slider_assoc[slider.owner] = target;
                continue;
            }
            else if ( ! strncmp(line, "desc:", 5) ) {
            	char *src = line+5;
            	src = trim(src, true, true);
                strncpy(desc, src, 64);
                continue;
            }
            else if ( ! strnicmp(line, "filename:", 9) ) {
				
            	// filename:0,filename.wav
				
            	char *src = line+8;
				
            	src = trim(src, true, false);
				
            	if ( *src != ':' )
            		return false;
				src++;
				
				src = trim(src, true, false);
				
				int index;
				if ( sscanf(src, "%d", &index) != 1 )
					return false;
				while ( isdigit(*src) )
					src++;
				
				src = trim(src, true, false);
				
				if ( *src != ',' )
					return false;
				src++;
				
				src = trim(src, true, true);
				
				std::string resolvedPath;
				if ( pathLibrary.resolveImportPath(src, path, resolvedPath) ) {
					if ( ! handleFile(index, resolvedPath.c_str() ) ) {
						return false;
					}
				}
            }
            else if ( ! strncmp(line, "import ", 7) ) {
				char *src = line+7;
				src = trim(src, true, true);
					
				if (*src) {
					processImport(pathLibrary, path, src, sections, compileFlags);
				}
                continue;
            }
            else if ( ! strncmp(line, "in_pin:", 7) ) {
				char *src = line+7;
				src = trim(src, true, true);
                if ( ! strncmp(src, "none", 4) ) {
                    inputNames.clear();
                    numInputs = -1;
                } else {
                    if ( numInputs != -1 ) {
                        inputNames.push_back(src);
                        numInputs++;
                    }
                }
            }
            else if ( ! strncmp(line, "out_pin:", 8) ) {
				char *src = line+8;
				src = trim(src, true, true);
                if ( ! strncmp(src, "none", 4) ) {
                    outputNames.clear();
                    numOutputs = -1;
                } else {
                    if ( numOutputs != -1 ) {
                        outputNames.push_back(src);
                        numOutputs++;
                    }
                }
            }
        }
    }
	
	return true;
}

bool JsusFx::compileSections(JsusFx_Sections &sections, const int compileFlags) {
	bool result = true;
	
	// 0 init
	// 1 slider
	// 2 block
	// 3 sample
	// 4 gfx
	// 5 serialize
	
	if (sections.init.code.GetLength() != 0)
		result &= compileSection(0, sections.init.code.Get(), sections.init.lineOffset);
	if (sections.slider.code.GetLength() != 0)
		result &= compileSection(1, sections.slider.code.Get(), sections.slider.lineOffset);
	if (sections.block.code.GetLength() != 0)
		result &= compileSection(2, sections.block.code.Get(), sections.block.lineOffset);
	if (sections.sample.code.GetLength() != 0)
		result &= compileSection(3, sections.sample.code.Get(), sections.sample.lineOffset);
	if (sections.gfx.code.GetLength() != 0 && (compileFlags & kCompileFlag_CompileGraphicsSection) != 0)
		result &= compileSection(4, sections.gfx.code.Get(), sections.gfx.lineOffset);
	if (sections.serialize.code.GetLength() != 0 && (compileFlags & kCompileFlag_CompileSerializeSection) != 0)
		result &= compileSection(5, sections.serialize.code.Get(), sections.serialize.lineOffset);
	
	if (result == false)
		releaseCode();
	
	return result;
}

bool JsusFx::compile(JsusFxPathLibrary &pathLibrary, const std::string &path, const int compileFlags) {
	releaseCode();
	
	std::string resolvedPath;
	if ( ! pathLibrary.resolveImportPath(path, "", resolvedPath) ) {
 		displayError("Failed to open %s", path.c_str());
 		return false;
 	}
	
	std::istream *input = pathLibrary.open(resolvedPath);
	if ( input == nullptr ) {
		displayError("Failed to open %s", resolvedPath.c_str());
		return false;
	}
	
	// read code for the various sections inside the jsusfx script
	
	JsusFx_Sections sections;
	if ( ! readSections(pathLibrary, resolvedPath, *input, sections, compileFlags) )
		return false;
	
	pathLibrary.close(input);
	
	// compile the sections
	
	if ( ! compileSections(sections, compileFlags) ) {
		releaseCode();
		return false;
	}
	
	computeSlider = 1;
    
    // in_pin and out_pin is optional, we default it to 2 in / 2 out if nothing is specified.
    // if you really want no in or out, specify in_pin:none/out_pin:none
    if ( numInputs == 0 )
        numInputs = 2;
    else if ( numInputs == -1 )
        numInputs = 0;
    
    if ( numOutputs == 0 )
        numOutputs = 2;
    else if ( numOutputs == -1 )
        numOutputs = 0;
    
    return true;
}

bool JsusFx::readHeader(JsusFxPathLibrary &pathLibrary, const std::string &path) {
	std::string resolvedPath;
	if ( ! pathLibrary.resolveImportPath(path, "", resolvedPath) ) {
 		displayError("Failed to open %s", path.c_str());
 		return false;
 	}
	
	std::istream *input = pathLibrary.open(resolvedPath);
	if ( input == nullptr ) {
		displayError("Failed to open %s", resolvedPath.c_str());
		return false;
	}
	
	if ( ! readHeader(pathLibrary, resolvedPath, *input) )
		return false;
	
	pathLibrary.close(input);
	
	return true;
}

void JsusFx::prepare(int sampleRate, int blockSize) {    
    *srate = (double) sampleRate;
    *samplesblock = blockSize;
    NSEEL_code_execute(codeInit);
    computeSlider = true;
}

void JsusFx::moveSlider(int idx, float value, int normalizeSlider) {
    if ( idx < 0 || idx >= kMaxSliders || !sliders[idx].exists )
        return;

    if ( normalizeSlider != 0 ) {
        float steps = sliders[idx].max - sliders[idx].min;
        value  = (value * steps) / normalizeSlider;
        value += sliders[idx].min;
    }

    if ( sliders[idx].inc != 0 ) {
        int tmp = roundf(value / sliders[idx].inc);
        value = sliders[idx].inc * tmp;
    }

    computeSlider |= sliders[idx].setValue(value);
}

bool JsusFx::addInputEvent(int offset, const uint8_t *data, int length)
{
	EventHeader event;
	event.offset = offset;
	event.length = length;
	std::copy((char*)&event, (char*)&event + sizeof(event), std::back_inserter(midiInput));
	std::copy(data, data + length, std::back_inserter(midiInput));
	return true;
}

bool JsusFx::addOutputEvent(int offset, const uint8_t *data, int length)
{
	EventHeader event;
	event.offset = offset;
	event.length = length;
	std::copy((char*)&event, (char*)&event + sizeof(event), std::back_inserter(midiOutput));
	std::copy(data, data + length, std::back_inserter(midiOutput));
	return true;
}

bool JsusFx::iterateOutputEvents(size_t &iterPos, int &offset, const uint8_t *&data, int &length)
{
	if (iterPos < midiOutput.size()) {
		EventHeader event;
		data = &midiOutput[iterPos];
		memcpy(&event, data, sizeof(event));
		data += sizeof(event);
		iterPos += sizeof(event) + event.length;
		
		offset = event.offset;
		length = event.length;
		return true;
	}
	else {
		return false;
	}
}

void JsusFx::setTransportValues(
	const double in_tempo,
	const PlaybackState in_playbackState,
	const double in_playbackPositionInSeconds,
	const double in_beatPosition,
	const int in_timeSignatureNumerator,
	const int in_timeSignatureDenumerator)
{
	*tempo = in_tempo;
	*play_state = in_playbackState;
	*play_position = in_playbackPositionInSeconds;
	*beat_position = in_beatPosition;
	*ts_num = in_timeSignatureNumerator;
	*ts_denom = in_timeSignatureDenumerator;
}

bool JsusFx::process(const float **input, float **output, int size, int numInputChannels, int numOutputChannels) {
    midiInputReadPos = 0;
    midiOutput.clear();

    sliderchangebits = 0;

    if ( computeSlider ) {
        NSEEL_code_execute(codeSlider);
        computeSlider = false;      
    }
	
    numValidInputChannels = numInputChannels;
	
    *samplesblock = size;
    *num_ch = numValidInputChannels;
    NSEEL_code_execute(codeBlock);

    if (codeSample) {
        for(int i=0;i<size;i++) {
            for (int c = 0; c < numInputChannels; ++c)
                *spl[c] = input[c][i];
            NSEEL_code_execute(codeSample);
            for (int c = 0; c < numOutputChannels; ++c)
                output[c][i] = *spl[c];
        }
    }
	
	midiInput.clear();
	
    return true;
}

bool JsusFx::process64(const double **input, double **output, int size, int numInputChannels, int numOutputChannels) {
    midiInputReadPos = 0;
    midiOutput.clear();

    sliderchangebits = 0;

    if ( computeSlider ) {
        NSEEL_code_execute(codeSlider);
        computeSlider = false;
    }
	
    numValidInputChannels = numInputChannels;

    *samplesblock = size;
    *num_ch = numValidInputChannels;
    NSEEL_code_execute(codeBlock);

    if (codeSample) {
        for(int i=0;i<size;i++) {
            for (int c = 0; c < numInputChannels; ++c)
                *spl[c] = input[c][i];
            NSEEL_code_execute(codeSample);
            for (int c = 0; c < numOutputChannels; ++c)
                output[c][i] = *spl[c];
        }
    }
	
	midiInput.clear();
	
    return true;
}

void JsusFx::draw() {
    if ( codeGfx == NULL )
        return;
	
	if ( gfx != nullptr )
		gfx->beginDraw();
	
	NSEEL_code_execute(codeGfx);
	
	if ( gfx != nullptr )
		gfx->endDraw();
}

bool JsusFx::serialize(JsusFxSerializer & _serializer, const bool write) {
	serializer = &_serializer;
	serializer->begin(*this, write);
	{
		if (codeSerialize != nullptr)
			NSEEL_code_execute(codeSerialize);
		
		if (write == false)
		{
			if (codeSlider != nullptr)
				NSEEL_code_execute(codeSlider);
			computeSlider = false;
		}
	}
	serializer->end();
	serializer = nullptr;
	
	return true;
}

const char * JsusFx::getString(const int index, WDL_FastString ** fs) {
	void * opaque = this;
	return EEL_STRING_GET_FOR_INDEX(index, fs);
}

bool JsusFx::handleFile(int index, const char *filename)
{
	if (index < 0 || index >= kMaxFileInfos)
	{
		displayError("file index out of bounds %d:%s", index, filename);
		return false;
	}
	
	if (fileInfos[index].isValid())
	{
		displayMsg("file already exists %d:%s", index, filename);
		
		fileInfos[index] = JsusFx_FileInfo();
	}
	
	//
	
	if (fileInfos[index].init(filename))
	{
		const char *ext = nullptr;
		for (const char *p = filename; *p; ++p)
			if (*p == '.')
				ext = p + 1;
		
		if (ext != nullptr && (stricmp(ext, "png") == 0 || stricmp(ext, "jpg") == 0))
		{
			if (gfx != nullptr)
			{
				gfx->gfx_loadimg(*this, index, index);
			}
		}
		
		return true;
	}
	else
	{
		displayError("failed to find file %d:%s", index, filename);
		
		return false;
	}
}

void JsusFx::releaseCode() {
    desc[0] = 0;
    
    if ( codeInit )
        NSEEL_code_free(codeInit);
    if ( codeSlider ) 
        NSEEL_code_free(codeSlider);
    if ( codeBlock  ) 
        NSEEL_code_free(codeBlock);
    if ( codeSample ) 
        NSEEL_code_free(codeSample);
	if ( codeGfx )
		NSEEL_code_free(codeGfx);
        
    codeInit = codeSlider = codeBlock = codeSample = codeGfx = codeSerialize = NULL;
	
    NSEEL_code_compile_ex(m_vm, nullptr, 0, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS_RESET);

	numInputs = 0;
	numOutputs = 0;
	
    for(int i=0;i<kMaxSliders;i++)
    	sliders[i].exists = false;
    var_slider_assoc.clear();
	
	gfx_w = 0;
	gfx_h = 0;
	
    NSEEL_VM_remove_unused_vars(m_vm);
    NSEEL_VM_remove_all_nonreg_vars(m_vm);
}

static WDL_Mutex initMutex;
static volatile bool initFlag = false;

void JsusFx::init() {
    if (initFlag)
        return;

    WDL_MutexLock lock(&initMutex);
    if (initFlag)
        return;

    EEL_string_register();
    EEL_fft_register();
    EEL_mdct_register();
    EEL_string_register();
    EEL_misc_register();
    EEL_atomic_register();

    initFlag = true;
}

static int dumpvarsCallback(const char *name, EEL_F *val, void *ctx) {
    JsusFx *fx = (JsusFx *) ctx;
    int target;
        
    if ( sscanf(name, "slider%d", &target) ) {
        if ( target >= 0 && target < JsusFx::kMaxSliders ) {
            if ( ! fx->sliders[target].exists ) {
                return 1;
            } else {
                fx->displayMsg("%s --> %f (%s)", name, *val, fx->sliders[target].desc);
                return 1;
            }
        }
    }
    
    fx->displayMsg("%s --> %f", name, *val);
    return 1;
}

void JsusFx::dumpvars() {
    NSEEL_VM_enumallvars(m_vm, dumpvarsCallback, this);
}

#ifndef JSUSFX_OWNSCRIPTMUTEXT
// todo : implement mutex interface ?
void NSEEL_HOSTSTUB_EnterMutex() { }
void NSEEL_HOSTSTUB_LeaveMutex() { }
#endif

#ifdef _WIN32
static std::wstring toWideString(const std::string &u8str)
{
    std::wstring wstr;
    int wch = MultiByteToWideChar(CP_UTF8, 0, u8str.data(), (int)u8str.size(), nullptr, 0);
    if (wch != 0) {
        wstr.resize((size_t)wch);
        MultiByteToWideChar(CP_UTF8, 0, u8str.data(), (int)u8str.size(), &wstr[0], wch);
    }
    return wstr;
}
#endif
