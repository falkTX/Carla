/*
  Copyright 2012-2014 David Robillard <http://drobilla.net>

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

   PUGL_HAVE_CAIRO: Include Cairo support code.
   PUGL_HAVE_GL:    Include OpenGL support code.
   PUGL_GRAB_FOCUS: Work around reparent keyboard issues by grabbing focus.
   PUGL_VERBOSE:    Print GL information to console.
*/

#include "pugl/pugl.h"
#include "pugl/event.h"

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
	PuglFileSelectedFunc fileSelectedFunc;

	PuglInternals* impl;

	PuglNativeWindow parent;
	PuglContextType  ctx_type;
	uintptr_t        transient_parent;

	int      width;
	int      height;
	int      min_width;
	int      min_height;
	int      mods;
	bool     mouse_in_view;
	bool     ignoreKeyRepeat;
	bool     redisplay;
	bool     resizable;
	uint32_t event_timestamp_ms;
};

PuglInternals* puglInitInternals(void);

PuglView*
puglInit(void)
{
	PuglView* view = (PuglView*)calloc(1, sizeof(PuglView));
	if (!view) {
		return NULL;
	}

	PuglInternals* impl = puglInitInternals();
	if (!impl) {
		free(view);
		return NULL;
	}

	view->impl   = impl;
	view->width  = 640;
	view->height = 480;

	return view;
}

void
puglInitWindowSize(PuglView* view, int width, int height)
{
	view->width  = width;
	view->height = height;
}

void
puglInitWindowMinSize(PuglView* view, int width, int height)
{
	view->min_width  = width;
	view->min_height = height;
}

void
puglInitWindowParent(PuglView* view, PuglNativeWindow parent)
{
	view->parent = parent;
}

void
puglInitUserResizable(PuglView* view, bool resizable)
{
	view->resizable = resizable;
}

void
puglInitTransientFor(PuglView* view, uintptr_t parent)
{
	view->transient_parent = parent;
}

void
puglInitContextType(PuglView* view, PuglContextType type)
{
	view->ctx_type = type;
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

void
puglSetFileSelectedFunc(PuglView* view, PuglFileSelectedFunc fileSelectedFunc)
{
	view->fileSelectedFunc = fileSelectedFunc;
}

void
puglEnterContext(PuglView* view);

void
puglLeaveContext(PuglView* view, bool flush);

#if 0
/** Return the code point for buf, or the replacement character on error. */
static uint32_t
puglDecodeUTF8(const uint8_t* buf)
{
#define FAIL_IF(cond) { if (cond) return 0xFFFD; }

	/* http://en.wikipedia.org/wiki/UTF-8 */

	if (buf[0] < 0x80) {
		return buf[0];
	} else if (buf[0] < 0xC2) {
		return 0xFFFD;
	} else if (buf[0] < 0xE0) {
		FAIL_IF((buf[1] & 0xC0) != 0x80);
		return (buf[0] << 6) + buf[1] - 0x3080;
	} else if (buf[0] < 0xF0) {
		FAIL_IF((buf[1] & 0xC0) != 0x80);
		FAIL_IF(buf[0] == 0xE0 && buf[1] < 0xA0);
		FAIL_IF((buf[2] & 0xC0) != 0x80);
		return (buf[0] << 12) + (buf[1] << 6) + buf[2] - 0xE2080;
	} else if (buf[0] < 0xF5) {
		FAIL_IF((buf[1] & 0xC0) != 0x80);
		FAIL_IF(buf[0] == 0xF0 && buf[1] < 0x90);
		FAIL_IF(buf[0] == 0xF4 && buf[1] >= 0x90);
		FAIL_IF((buf[2] & 0xC0) != 0x80);
		FAIL_IF((buf[3] & 0xC0) != 0x80);
		return ((buf[0] << 18) +
		        (buf[1] << 12) +
		        (buf[2] << 6) +
		        buf[3] - 0x3C82080);
	}
	return 0xFFFD;
}
#endif

static void
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

#if 0
static void
puglDispatchEvent(PuglView* view, const PuglEvent* event)
{
	if (view->eventFunc) {
		view->eventFunc(view, event);
	}

	switch (event->type) {
	case PUGL_CONFIGURE:
		puglEnterContext(view);
		view->width  = event->configure.width;
		view->height = event->configure.height;
		if (view->reshapeFunc) {
			view->reshapeFunc(view, view->width, view->height);
		}
		puglLeaveContext(view, false);
		break;
	case PUGL_EXPOSE:
		if (event->expose.count == 0) {
			puglEnterContext(view);
			if (view->displayFunc) {
				view->displayFunc(view);
			}
			view->redisplay = false;
			puglLeaveContext(view, true);
		}
		break;
	case PUGL_MOTION_NOTIFY:
		view->event_timestamp_ms = event->motion.time;
		view->mods               = event->motion.state;
		if (view->motionFunc) {
			view->motionFunc(view, event->motion.x, event->motion.y);
		}
		break;
	case PUGL_SCROLL:
		if (view->scrollFunc) {
			view->scrollFunc(view,
			                 event->scroll.x, event->scroll.y,
			                 event->scroll.dx, event->scroll.dy);
		}
		break;
	case PUGL_BUTTON_PRESS:
	case PUGL_BUTTON_RELEASE:
		view->event_timestamp_ms = event->button.time;
		view->mods               = event->button.state;
		if (view->mouseFunc) {
			view->mouseFunc(view,
			                event->button.button,
			                event->type == PUGL_BUTTON_PRESS,
			                event->button.x,
			                event->button.y);
		}
		break;
	case PUGL_KEY_PRESS:
	case PUGL_KEY_RELEASE:
		view->event_timestamp_ms = event->key.time;
		view->mods               = event->key.state;
		if (event->key.special && view->specialFunc) {
			view->specialFunc(view,
			                  event->type == PUGL_KEY_PRESS,
			                  event->key.special);
		} else if (event->key.character && view->keyboardFunc) {
			view->keyboardFunc(view,
			                   event->type == PUGL_KEY_PRESS,
			                   event->key.character);
		}
		break;
	default:
		break;
	}
}
#endif
