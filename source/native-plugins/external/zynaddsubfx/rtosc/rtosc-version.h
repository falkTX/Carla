/*
 * Copyright (c) 2017 Johannes Lorenz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * @file rtosc-version.h
 * Definition of rtosc's version struct
 * @note the implementation is in version.c.in
 */

#ifndef RTOSC_VERSION_H
#define RTOSC_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

//! @brief struct containing an rtosc version
typedef struct
{
    unsigned char major;
    unsigned char minor;
    unsigned char revision;
} rtosc_version;

//! @brief memcmp-like version compare
//! @return an integer greater than, equal to, or less than 0, if
//!         v1 is greater than, equal to, or less than v2, respectively
int rtosc_version_cmp(const rtosc_version v1,
                      const rtosc_version v2);

//! @brief Return the version RT OSC has been compiled with
rtosc_version rtosc_current_version();

//! @brief Print the version pointed to by @p v to the buffer @p _12bytes
//!
//! The format "<major>.<minor>.<revision>" is being used
//! @param v Pointer to the version that shall be printed
//! @param _12bytes A buffer with a size of at least 12 bytes
void rtosc_version_print_to_12byte_str(const rtosc_version* v,
                                       char* _12bytes);

#ifdef __cplusplus
};
#endif

#endif

