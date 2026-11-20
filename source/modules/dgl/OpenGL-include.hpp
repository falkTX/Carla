/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DGL_OPENGL_INCLUDE_HPP_INCLUDED
#define DGL_OPENGL_INCLUDE_HPP_INCLUDED

#include "../distrho/src/DistrhoDefines.h"

// --------------------------------------------------------------------------------------------------------------------
// Fix OpenGL includes for Windows, based on glfw code (part 1)

#undef DGL_WINGDIAPI_DEFINED

#ifdef DISTRHO_OS_WINDOWS

#ifndef WINAPI
# define WINAPI __stdcall
#endif

#ifndef APIENTRY
# define APIENTRY WINAPI
#endif // APIENTRY

/* We need WINGDIAPI defined */
#ifndef WINGDIAPI
# if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__POCC__)
#  define WINGDIAPI __declspec(dllimport)
# elif defined(__LCC__)
#  define WINGDIAPI __stdcall
# else
#  define WINGDIAPI extern
# endif
# define DGL_WINGDIAPI_DEFINED
#endif // WINGDIAPI

#endif // DISTRHO_OS_WINDOWS

// --------------------------------------------------------------------------------------------------------------------
// OpenGL includes

#ifdef DISTRHO_OS_MAC
# ifdef DGL_USE_OPENGL3
// NOTE GLES with macOS is not supported
#  ifdef DGL_USE_GLES
#   undef DGL_USE_GLES
#   undef DGL_USE_GLES2
#   undef DGL_USE_GLES3
#  endif
#  include <OpenGL/gl3.h>
#  include <OpenGL/gl3ext.h>
# else
#  include <OpenGL/gl.h>
# endif
#else
# ifndef DISTRHO_OS_WINDOWS
#  define GL_GLEXT_PROTOTYPES
# endif
# ifndef __GLEW_H__
#  include <GL/gl.h>
#  include <GL/glext.h>
# endif
#endif

// --------------------------------------------------------------------------------------------------------------------
// Missing OpenGL defines

#if defined(GL_BGR_EXT) && !defined(GL_BGR)
# define GL_BGR GL_BGR_EXT
#endif

#if defined(GL_BGRA_EXT) && !defined(GL_BGRA)
# define GL_BGRA GL_BGRA_EXT
#endif

#ifndef GL_CLAMP_TO_BORDER
# define GL_CLAMP_TO_BORDER 0x812D
#endif

// --------------------------------------------------------------------------------------------------------------------
// Fix OpenGL includes for Windows, based on glfw code (part 2)

#ifdef DGL_WINGDIAPI_DEFINED
# undef WINGDIAPI
# undef DGL_WINGDIAPI_DEFINED
#endif

// --------------------------------------------------------------------------------------------------------------------

#endif
