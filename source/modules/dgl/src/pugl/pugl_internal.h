/*
  Copyright 2012 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file pugl_internal.h Private platform-independent definitions.

   Note this file contains function definitions, so it must be compiled into
   the final binary exactly once.  Each platform specific implementation file
   including it once should achieve this.

   If you are copying the pugl code into your source tree, the following
   symbols can be defined to tweak pugl behaviour:

   PUGL_GRAB_FOCUS: Work around reparent keyboard issues by grabbing focus.
   PUGL_VERBOSE:    Print GL information to console.
*/

#include "pugl.h"

#ifdef PUGL_VERBOSE
#    include <stdio.h>
#    define PUGL_LOG(str)       fprintf(stderr, "pugl: " str)
#    define PUGL_LOGF(fmt, ...) fprintf(stderr, "pugl: " fmt, __VA_ARGS__)
#else
#    define PUGL_LOG(str)
#    define PUGL_LOGF(fmt, ...)
#endif

typedef struct PuglInternalsImpl PuglInternals;

struct PuglViewImpl {
	PuglHandle       handle;
	PuglCloseFunc    closeFunc;
	PuglDisplayFunc  displayFunc;
	PuglKeyboardFunc keyboardFunc;
	PuglMotionFunc   motionFunc;
	PuglMouseFunc    mouseFunc;
	PuglReshapeFunc  reshapeFunc;
	PuglScrollFunc   scrollFunc;
	PuglSpecialFunc  specialFunc;

	PuglInternals* impl;

	PuglNativeWindow parent;

	int      width;
	int      height;
	int      mods;
	bool     mouse_in_view;
	bool     ignoreKeyRepeat;
	bool     redisplay;
	bool     resizable;
	uint32_t event_timestamp_ms;
};

PuglInternals* puglInitInternals();

void puglDefaultReshape(PuglView* view, int width, int height);

PuglView*
puglInit(int* pargc, char** argv)
{
	PuglView* view = (PuglView*)calloc(1, sizeof(PuglView));
	if (!view) {
		return NULL;
	}

	PuglInternals* impl = puglInitInternals();
	if (!impl) {
		return NULL;
	}

	view->impl   = impl;
	view->width  = 640;
	view->height = 480;

	return view;

	// unused
	(void)pargc; (void)argv;
}

void
puglInitWindowSize(PuglView* view, int width, int height)
{
	view->width  = width;
	view->height = height;
}

void
puglInitWindowParent(PuglView* view, PuglNativeWindow parent)
{
	view->parent = parent;
}

void
puglInitResizable(PuglView* view, bool resizable)
{
	view->resizable = resizable;
}

void
puglSetHandle(PuglView* view, PuglHandle handle)
{
	view->handle = handle;
}

PuglHandle
puglGetHandle(PuglView* view)
{
	return view->handle;
}

uint32_t
puglGetEventTimestamp(PuglView* view)
{
	return view->event_timestamp_ms;
}

int
puglGetModifiers(PuglView* view)
{
	return view->mods;
}

void
puglDefaultReshape(PuglView* view, int width, int height)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, 0, 1);
	glViewport(0, 0, width, height);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	return;

	// unused
	(void)view;
}

void
puglIgnoreKeyRepeat(PuglView* view, bool ignore)
{
	view->ignoreKeyRepeat = ignore;
}

void
puglSetCloseFunc(PuglView* view, PuglCloseFunc closeFunc)
{
	view->closeFunc = closeFunc;
}

void
puglSetDisplayFunc(PuglView* view, PuglDisplayFunc displayFunc)
{
	view->displayFunc = displayFunc;
}

void
puglSetKeyboardFunc(PuglView* view, PuglKeyboardFunc keyboardFunc)
{
	view->keyboardFunc = keyboardFunc;
}

void
puglSetMotionFunc(PuglView* view, PuglMotionFunc motionFunc)
{
	view->motionFunc = motionFunc;
}

void
puglSetMouseFunc(PuglView* view, PuglMouseFunc mouseFunc)
{
	view->mouseFunc = mouseFunc;
}

void
puglSetReshapeFunc(PuglView* view, PuglReshapeFunc reshapeFunc)
{
	view->reshapeFunc = reshapeFunc;
}

void
puglSetScrollFunc(PuglView* view, PuglScrollFunc scrollFunc)
{
	view->scrollFunc = scrollFunc;
}

void
puglSetSpecialFunc(PuglView* view, PuglSpecialFunc specialFunc)
{
	view->specialFunc = specialFunc;
}
