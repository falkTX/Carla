/*
  Copyright 2012 David Robillard <http://drobilla.net>
  Copyright 2011-2012 Ben Loftis, Harrison Consoles

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

#include "pugl_x11.h"
#include "pugl_internal.h"

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
	None
};

PuglView*
puglCreate(PuglNativeWindow parent,
           const char*      title,
           int              width,
           int              height,
           bool             resizable,
           bool             addToDesktop,
           const char*      x11Display)
{
	PuglView*      view = (PuglView*)calloc(1, sizeof(PuglView));
	PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));
	if (!view || !impl) {
		return NULL;
	}

	view->impl   = impl;
	view->width  = width;
	view->height = height;

	impl->display = XOpenDisplay(x11Display);
	impl->screen  = DefaultScreen(impl->display);

	XVisualInfo* vi = glXChooseVisual(impl->display, impl->screen, attrListDbl);
	if (!vi) {
		vi = glXChooseVisual(impl->display, impl->screen, attrListSgl);
		impl->doubleBuffered = False;
		printf("singlebuffered rendering will be used, no doublebuffering available\n");
	} else {
		impl->doubleBuffered = True;
		printf("doublebuffered rendering available\n");
	}

	int glxMajor, glxMinor;
	glXQueryVersion(impl->display, &glxMajor, &glxMinor);
	printf("GLX-Version %d.%d\n", glxMajor, glxMinor);

	impl->ctx = glXCreateContext(impl->display, vi, 0, GL_TRUE);

	Window xParent = parent
		? (Window)parent
		: RootWindow(impl->display, impl->screen);

	Colormap cmap = XCreateColormap(
		impl->display, xParent, vi->visual, AllocNone);

	XSetWindowAttributes attr;
	memset(&attr, 0, sizeof(XSetWindowAttributes));
	attr.colormap     = cmap;
	attr.border_pixel = 0;

	attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask
		| ButtonPressMask | ButtonReleaseMask
		| PointerMotionMask | StructureNotifyMask;

	impl->win = XCreateWindow(
		impl->display, xParent,
		0, 0, view->width, view->height, 0, vi->depth, InputOutput, vi->visual,
		CWBorderPixel | CWColormap | CWEventMask, &attr);

	XSizeHints sizeHints;
	memset(&sizeHints, 0, sizeof(sizeHints));
	if (!resizable) {
		sizeHints.flags      = PMinSize|PMaxSize;
		sizeHints.min_width  = width;
		sizeHints.min_height = height;
		sizeHints.max_width  = width;
		sizeHints.max_height = height;
		XSetNormalHints(impl->display, impl->win, &sizeHints);
	}

	if (title) {
		XStoreName(impl->display, impl->win, title);
	}

	if (!parent) {
		Atom wmDelete = XInternAtom(impl->display, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(impl->display, impl->win, &wmDelete, 1);
	}

	if (addToDesktop) {
		XMapRaised(impl->display, impl->win);
	}

	if (glXIsDirect(impl->display, impl->ctx)) {
		printf("DRI enabled\n");
	} else {
		printf("no DRI available\n");
	}

	XFree(vi);

	return view;
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	if (view->displayFunc) {
		view->displayFunc(view);
	}

	glFlush();
	if (view->impl->doubleBuffered) {
		glXSwapBuffers(view->impl->display, view->impl->win);
	}

	view->redisplay = false;
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
setModifiers(PuglView* view, int xstate)
{
	view->mods = 0;
	view->mods |= (xstate & ShiftMask)   ? PUGL_MOD_SHIFT  : 0;
	view->mods |= (xstate & ControlMask) ? PUGL_MOD_CTRL   : 0;
	view->mods |= (xstate & Mod1Mask)    ? PUGL_MOD_ALT    : 0;
	view->mods |= (xstate & Mod4Mask)    ? PUGL_MOD_SUPER  : 0;
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	XEvent event;
	while (XPending(view->impl->display) > 0) {
		XNextEvent(view->impl->display, &event);
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
			view->redisplay = false;
			break;
		case MotionNotify:
			setModifiers(view, event.xmotion.state);
			if (view->motionFunc) {
				view->motionFunc(view, event.xmotion.x, event.xmotion.y);
			}
			break;
		case ButtonPress:
			setModifiers(view, event.xbutton.state);
			if (event.xbutton.button >= 4 && event.xbutton.button <= 7) {
				if (view->scrollFunc) {
					float dx = 0, dy = 0;
					switch (event.xbutton.button) {
					case 4: dy =  1.0f; break;
					case 5: dy = -1.0f; break;
					case 6: dx = -1.0f; break;
					case 7: dx =  1.0f; break;
					}
					view->scrollFunc(view, dx, dy);
				}
				break;
			}
			// nobreak
		case ButtonRelease:
			setModifiers(view, event.xbutton.state);
			if (view->mouseFunc &&
			    (event.xbutton.button < 4 || event.xbutton.button > 7)) {
				view->mouseFunc(view,
				                event.xbutton.button, event.type == ButtonPress,
				                event.xbutton.x, event.xbutton.y);
			}
			break;
		case KeyPress: {
			setModifiers(view, event.xkey.state);
			KeySym  sym;
			char    str[5];
			int     n   = XLookupString(&event.xkey, str, 4, &sym, NULL);
			PuglKey key = keySymToSpecial(sym);
			if (!key && view->keyboardFunc) {
				if (n == 1) {
					view->keyboardFunc(view, true, str[0]);
				} else {
					fprintf(stderr, "warning: Unknown key %X\n", (int)sym);
				}
			} else if (view->specialFunc) {
				view->specialFunc(view, true, key);
			}
		} break;
		case KeyRelease: {
			setModifiers(view, event.xkey.state);
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

			if (!repeated && view->keyboardFunc) {
				KeySym sym = XKeycodeToKeysym(
					view->impl->display, event.xkey.keycode, 0);
				PuglKey special = keySymToSpecial(sym);
				if (!special) {
					view->keyboardFunc(view, false, sym);
				} else if (view->specialFunc) {
					view->specialFunc(view, false, special);
				}
			}
		} break;
		case ClientMessage:
			if (!strcmp(XGetAtomName(view->impl->display,
			                         event.xclient.message_type),
			            "WM_PROTOCOLS")) {
				if (view->closeFunc) {
					view->closeFunc(view);
				}
			}
			break;
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

PuglInternals*
puglGetInternalsImpl(PuglView* view)
{
	return view->impl;
}
