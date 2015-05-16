/*
  Copyright 2012-2014 David Robillard <http://drobilla.net>
  Copyright 2011-2012 Ben Loftis, Harrison Consoles
  Copyright 2013 Robin Gareus <robin@gareus.org>

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
   @file pugl_x11.c X11 Pugl Implementation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "pugl_internal.h"

#define SOFD_HAVE_X11
#include "../sofd/libsofd.h"
#include "../sofd/libsofd.c"

struct PuglInternalsImpl {
	Display*   display;
	int        screen;
	Window     win;
	GLXContext ctx;
	Bool       doubleBuffered;
};

/**
   Attributes for single-buffered RGBA with at least
   4 bits per color and a 16 bit depth buffer.
*/
static int attrListSgl[] = {
	GLX_RGBA,
	GLX_RED_SIZE, 4,
	GLX_GREEN_SIZE, 4,
	GLX_BLUE_SIZE, 4,
	GLX_DEPTH_SIZE, 16,
	GLX_ARB_multisample, 1,
	None
};

/**
   Attributes for double-buffered RGBA with at least
   4 bits per color and a 16 bit depth buffer.
*/
static int attrListDbl[] = {
	GLX_RGBA, GLX_DOUBLEBUFFER,
	GLX_RED_SIZE, 4,
	GLX_GREEN_SIZE, 4,
	GLX_BLUE_SIZE, 4,
	GLX_DEPTH_SIZE, 16,
	GLX_ARB_multisample, 1,
	None
};

/**
   Attributes for double-buffered RGBA with multi-sampling
	 (antialiasing)
*/
static int attrListDblMS[] = {
	GLX_RGBA,
	GLX_DOUBLEBUFFER    , True,
	GLX_RED_SIZE        , 4,
	GLX_GREEN_SIZE      , 4,
	GLX_BLUE_SIZE       , 4,
	GLX_ALPHA_SIZE      , 4,
	GLX_DEPTH_SIZE      , 16,
	GLX_SAMPLE_BUFFERS  , 1,
	GLX_SAMPLES         , 4,
	None
};

PuglInternals*
puglInitInternals(void)
{
	return (PuglInternals*)calloc(1, sizeof(PuglInternals));
}

int
puglCreateWindow(PuglView* view, const char* title)
{
	PuglInternals* impl = view->impl;

	impl->display = XOpenDisplay(NULL);
	impl->screen  = DefaultScreen(impl->display);
	impl->doubleBuffered = True;

	XVisualInfo* vi = glXChooseVisual(impl->display, impl->screen, attrListDblMS);

	if (!vi) {
		vi = glXChooseVisual(impl->display, impl->screen, attrListDbl);
		PUGL_LOG("multisampling (antialiasing) is not available\n");
	}

	if (!vi) {
		vi = glXChooseVisual(impl->display, impl->screen, attrListSgl);
		impl->doubleBuffered = False;
		PUGL_LOG("singlebuffered rendering will be used, no doublebuffering available\n");
	}

	int glxMajor, glxMinor;
	glXQueryVersion(impl->display, &glxMajor, &glxMinor);
	PUGL_LOGF("GLX Version %d.%d\n", glxMajor, glxMinor);

	impl->ctx = glXCreateContext(impl->display, vi, 0, GL_TRUE);

	Window xParent = view->parent
		? (Window)view->parent
		: RootWindow(impl->display, impl->screen);

	Colormap cmap = XCreateColormap(
		impl->display, xParent, vi->visual, AllocNone);

	XSetWindowAttributes attr;
	memset(&attr, 0, sizeof(XSetWindowAttributes));
	attr.colormap     = cmap;
	attr.border_pixel = 0;

	attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask
		| ButtonPressMask | ButtonReleaseMask
#ifdef PUGL_GRAB_FOCUS
		| EnterWindowMask
#endif
		| PointerMotionMask | StructureNotifyMask;

	impl->win = XCreateWindow(
		impl->display, xParent,
		0, 0, view->width, view->height, 0, vi->depth, InputOutput, vi->visual,
		CWBorderPixel | CWColormap | CWEventMask, &attr);

	XSizeHints sizeHints;
	memset(&sizeHints, 0, sizeof(sizeHints));
	if (!view->resizable) {
		sizeHints.flags      = PMinSize|PMaxSize;
		sizeHints.min_width  = view->width;
		sizeHints.min_height = view->height;
		sizeHints.max_width  = view->width;
		sizeHints.max_height = view->height;
		XSetNormalHints(impl->display, impl->win, &sizeHints);
	}

	if (title) {
		XStoreName(impl->display, impl->win, title);
	}

	if (!view->parent) {
		Atom wmDelete = XInternAtom(impl->display, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(impl->display, impl->win, &wmDelete, 1);
	}

	if (glXIsDirect(impl->display, impl->ctx)) {
		PUGL_LOG("DRI enabled (to disable, set LIBGL_ALWAYS_INDIRECT=1\n");
	} else {
		PUGL_LOG("No DRI available\n");
	}

	XFree(vi);

	glXMakeCurrent(view->impl->display, view->impl->win, view->impl->ctx);

	return 0;
}

void
puglShowWindow(PuglView* view)
{
	PuglInternals* impl = view->impl;

	XMapRaised(impl->display, impl->win);
}

void
puglHideWindow(PuglView* view)
{
	PuglInternals* impl = view->impl;

	XUnmapWindow(impl->display, impl->win);
}

void
puglDestroy(PuglView* view)
{
	if (!view) {
		return;
	}

	glXDestroyContext(view->impl->display, view->impl->ctx);
	XDestroyWindow(view->impl->display, view->impl->win);
	XCloseDisplay(view->impl->display);
	free(view->impl);
	free(view);
}

static void
puglReshape(PuglView* view, int width, int height)
{
	glXMakeCurrent(view->impl->display, view->impl->win, view->impl->ctx);

	if (view->reshapeFunc) {
		view->reshapeFunc(view, width, height);
	} else {
		puglDefaultReshape(view, width, height);
	}

	view->width  = width;
	view->height = height;
}

static void
puglDisplay(PuglView* view)
{
	glXMakeCurrent(view->impl->display, view->impl->win, view->impl->ctx);

	view->redisplay = false;

	if (view->displayFunc) {
		view->displayFunc(view);
	}

	glFlush();

	if (view->impl->doubleBuffered) {
		glXSwapBuffers(view->impl->display, view->impl->win);
	}
}

static PuglKey
keySymToSpecial(KeySym sym)
{
	switch (sym) {
	case XK_F1:        return PUGL_KEY_F1;
	case XK_F2:        return PUGL_KEY_F2;
	case XK_F3:        return PUGL_KEY_F3;
	case XK_F4:        return PUGL_KEY_F4;
	case XK_F5:        return PUGL_KEY_F5;
	case XK_F6:        return PUGL_KEY_F6;
	case XK_F7:        return PUGL_KEY_F7;
	case XK_F8:        return PUGL_KEY_F8;
	case XK_F9:        return PUGL_KEY_F9;
	case XK_F10:       return PUGL_KEY_F10;
	case XK_F11:       return PUGL_KEY_F11;
	case XK_F12:       return PUGL_KEY_F12;
	case XK_Left:      return PUGL_KEY_LEFT;
	case XK_Up:        return PUGL_KEY_UP;
	case XK_Right:     return PUGL_KEY_RIGHT;
	case XK_Down:      return PUGL_KEY_DOWN;
	case XK_Page_Up:   return PUGL_KEY_PAGE_UP;
	case XK_Page_Down: return PUGL_KEY_PAGE_DOWN;
	case XK_Home:      return PUGL_KEY_HOME;
	case XK_End:       return PUGL_KEY_END;
	case XK_Insert:    return PUGL_KEY_INSERT;
	case XK_Shift_L:   return PUGL_KEY_SHIFT;
	case XK_Shift_R:   return PUGL_KEY_SHIFT;
	case XK_Control_L: return PUGL_KEY_CTRL;
	case XK_Control_R: return PUGL_KEY_CTRL;
	case XK_Alt_L:     return PUGL_KEY_ALT;
	case XK_Alt_R:     return PUGL_KEY_ALT;
	case XK_Super_L:   return PUGL_KEY_SUPER;
	case XK_Super_R:   return PUGL_KEY_SUPER;
	}
	return (PuglKey)0;
}

static void
setModifiers(PuglView* view, unsigned xstate, unsigned xtime)
{
	view->event_timestamp_ms = xtime;

	view->mods = 0;
	view->mods |= (xstate & ShiftMask)   ? PUGL_MOD_SHIFT  : 0;
	view->mods |= (xstate & ControlMask) ? PUGL_MOD_CTRL   : 0;
	view->mods |= (xstate & Mod1Mask)    ? PUGL_MOD_ALT    : 0;
	view->mods |= (xstate & Mod4Mask)    ? PUGL_MOD_SUPER  : 0;
}

static void
dispatchKey(PuglView* view, XEvent* event, bool press)
{
	KeySym    sym;
	char      str[5];
	const int n = XLookupString(&event->xkey, str, 4, &sym, NULL);

	if (sym == XK_Escape && view->closeFunc && !press && !view->parent) {
		view->closeFunc(view);
		view->redisplay = false;
		return;
	}
	if (n == 0) {
		return;
	}
	if (n > 1) {
		fprintf(stderr, "warning: Unsupported multi-byte key %X\n", (int)sym);
		return;
	}

	const PuglKey special = keySymToSpecial(sym);
	if (special && view->specialFunc) {
		view->specialFunc(view, press, special);
	} else if (!special && view->keyboardFunc) {
		view->keyboardFunc(view, press, str[0]);
	}
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	XEvent event;
	while (XPending(view->impl->display) > 0) {
		XNextEvent(view->impl->display, &event);

		if (x_fib_handle_events(view->impl->display, &event)) {
			const int status = x_fib_status();

			if (status > 0) {
				char* const filename = x_fib_filename();
				x_fib_close(view->impl->display);
				if (view->fileSelectedFunc) {
					view->fileSelectedFunc(view, filename);
				}
				free(filename);
			} else if (status < 0) {
				x_fib_close(view->impl->display);
				if (view->fileSelectedFunc) {
					view->fileSelectedFunc(view, NULL);
				}
			}
			break;
		}

		switch (event.type) {
		case MapNotify:
			puglReshape(view, view->width, view->height);
			break;
		case ConfigureNotify:
			if ((event.xconfigure.width != view->width) ||
			    (event.xconfigure.height != view->height)) {
				puglReshape(view,
				            event.xconfigure.width,
				            event.xconfigure.height);
			}
			break;
		case Expose:
			if (event.xexpose.count != 0) {
				break;
			}
			puglDisplay(view);
			break;
		case MotionNotify:
			setModifiers(view, event.xmotion.state, event.xmotion.time);
			if (view->motionFunc) {
				view->motionFunc(view, event.xmotion.x, event.xmotion.y);
			}
			break;
		case ButtonPress:
			setModifiers(view, event.xbutton.state, event.xbutton.time);
			if (event.xbutton.button >= 4 && event.xbutton.button <= 7) {
				if (view->scrollFunc) {
					float dx = 0, dy = 0;
					switch (event.xbutton.button) {
					case 4: dy =  1.0f; break;
					case 5: dy = -1.0f; break;
					case 6: dx = -1.0f; break;
					case 7: dx =  1.0f; break;
					}
					view->scrollFunc(view, event.xbutton.x, event.xbutton.y, dx, dy);
				}
				break;
			}
			// nobreak
		case ButtonRelease:
			setModifiers(view, event.xbutton.state, event.xbutton.time);
			if (view->mouseFunc &&
			    (event.xbutton.button < 4 || event.xbutton.button > 7)) {
				view->mouseFunc(view,
				                event.xbutton.button, event.type == ButtonPress,
				                event.xbutton.x, event.xbutton.y);
			}
			break;
		case KeyPress:
			setModifiers(view, event.xkey.state, event.xkey.time);
			dispatchKey(view, &event, true);
			break;
		case KeyRelease: {
			setModifiers(view, event.xkey.state, event.xkey.time);
			bool repeated = false;
			if (view->ignoreKeyRepeat &&
			    XEventsQueued(view->impl->display, QueuedAfterReading)) {
				XEvent next;
				XPeekEvent(view->impl->display, &next);
				if (next.type == KeyPress &&
				    next.xkey.time == event.xkey.time &&
				    next.xkey.keycode == event.xkey.keycode) {
					XNextEvent(view->impl->display, &event);
					repeated = true;
				}
			}
			if (!repeated) {
				dispatchKey(view, &event, false);
			}
		}	break;
		case ClientMessage: {
			char* type = XGetAtomName(view->impl->display,
			                          event.xclient.message_type);
			if (!strcmp(type, "WM_PROTOCOLS")) {
				if (view->closeFunc) {
					view->closeFunc(view);
					view->redisplay = false;
				}
			}
			XFree(type);
		}	break;
#ifdef PUGL_GRAB_FOCUS
		case EnterNotify:
			XSetInputFocus(view->impl->display, view->impl->win, RevertToPointerRoot, CurrentTime);
			break;
#endif
		default:
			break;
		}
	}

	if (view->redisplay) {
		puglDisplay(view);
	}

	return PUGL_SUCCESS;
}

void
puglPostRedisplay(PuglView* view)
{
	view->redisplay = true;
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return view->impl->win;
}
