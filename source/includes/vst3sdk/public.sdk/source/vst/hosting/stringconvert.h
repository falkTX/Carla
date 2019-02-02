//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    :
// Filename    : public.sdk/source/vst/hosting/stringconvert.h
// Created by  : Steinberg, 11/2014
// Description : c++11 unicode string convert functions
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2017, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/vsttypes.h"
#include <string>

//------------------------------------------------------------------------
namespace VST3 {
namespace StringConvert {

//------------------------------------------------------------------------
/**
 *  convert an UTF-8 string to an UTF-16 string
 *
 *  @param utf8Str UTF-8 string
 *
 *  @return UTF-16 string
 */
extern std::u16string convert (const std::string& utf8Str);

//------------------------------------------------------------------------
/**
 *  convert an UTF-8 string to an UTF-16 string buffer with max 127 characters
 *
 *  @param utf8Str UTF-8 string
 *  @param str     UTF-16 string buffer
 *
 *  @return true on success
 */
extern bool convert (const std::string& utf8Str, Steinberg::Vst::String128 str);

//------------------------------------------------------------------------
/**
 *  convert an UTF-8 string to an UTF-16 string buffer
 *
 *  @param utf8Str       UTF-8 string
 *  @param str           UTF-16 string buffer
 *  @param maxCharacters max characters that fit into str
 *
 *  @return true on success
 */
extern bool convert (const std::string& utf8Str, Steinberg::Vst::TChar* str,
                     uint32_t maxCharacters);

//------------------------------------------------------------------------
/**
 *  convert an UTF-16 string buffer to an UTF-8 string
 *
 *  @param str UTF-16 string buffer
 *
 *  @return UTF-8 string
 */
extern std::string convert (const Steinberg::Vst::TChar* str);

//------------------------------------------------------------------------
/**
 *  convert an UTF-16 string buffer to an UTF-8 string
 *
 *  @param str UTF-16 string buffer
 *	@param max maximum characters in str
 *
 *  @return UTF-8 string
 */
extern std::string convert (const Steinberg::Vst::TChar* str, uint32_t max);

//------------------------------------------------------------------------
/**
 *  convert an UTF-16 string to an UTF-8 string
 *
 *  @param str UTF-16 string
 *
 *  @return UTF-8 string
 */
extern std::string convert (const std::u16string& str);

//------------------------------------------------------------------------
/**
 *  convert a ASCII string buffer to an UTF-8 string
 *
 *  @param str ASCII string buffer
 *	@param max maximum characters in str
 *
 *  @return UTF-8 string
 */
extern std::string convert (const char* str, uint32_t max);

//------------------------------------------------------------------------
} // String

//------------------------------------------------------------------------
inline const Steinberg::Vst::TChar* toTChar (const std::u16string& str)
{
	return reinterpret_cast<const Steinberg::Vst::TChar*> (str.data ());
}

//------------------------------------------------------------------------
} // VST3
