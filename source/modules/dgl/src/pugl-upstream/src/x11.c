/*
  Copyright 2012-2021 David Robillard <d@drobilla.net>
  Copyright 2013 Robin Gareus <robin@gareus.org>
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

#ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 199309L
#endif

#include "x11.h"

#include "implementation.h"
#include "types.h"

#include "pugl/pugl.h"

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef HAVE_XRANDR
#  include <X11/extensions/Xrandr.h>
#endif

#ifdef HAVE_XSYNC
#  include <X11/extensions/sync.h>
#  include <X11/extensions/syncconst.h>
#endif

#ifdef HAVE_XCURSOR
#  include <X11/Xcursor/Xcursor.h>
#  include <X11/cursorfont.h>
#endif

#include <sys/select.h>
#include <sys/time.h>

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef MIN
#  define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#  define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifdef __cplusplus
#  define PUGL_INIT_STRUCT \
    {}
#else
#  define PUGL_INIT_STRUCT \
    {                      \
      0                    \
    }
#endif

enum WmClientStateMessageAction {
  WM_STATE_REMOVE,
  WM_STATE_ADD,
  WM_STATE_TOGGLE
};

#ifdef HAVE_XCURSOR
static const char* cursor_names_x11[] = {
  "left_ptr",          // ARROW
  "xterm",             // CARET
  "crosshair",         // CROSSHAIR
  "hand2",             // HAND
  "forbidden",         // NO
  "sb_h_double_arrow", // LEFT_RIGHT
  "sb_v_double_arrow", // UP_DOWN
  "size_fdiag",        // DIAGONAL
  "size_bdiag",        // ANTI_DIAGONAL
};

/*
static const char* cursor_names_xdg[] = {
  "default",     // ARROW
  "text",        // CARET
  "crosshair",   // CROSSHAIR
  "pointer",     // HAND
  "not-allowed", // NO
  "ew-resize",   // LEFT_RIGHT
  "ns-resize",   // UP_DOWN
  "nwse-resize", // DIAGONAL
  "nesw-resize", // ANTI_DIAGONAL
};
*/
#endif

static bool
initXSync(PuglWorldInternals* const impl)
{
#ifdef HAVE_XSYNC
  int                 syncMajor   = 0;
  int                 syncMinor   = 0;
  int                 errorBase   = 0;
  XSyncSystemCounter* counters    = NULL;
  int                 numCounters = 0;

  if (XSyncQueryExtension(impl->display, &impl->syncEventBase, &errorBase) &&
      XSyncInitialize(impl->display, &syncMajor, &syncMinor) &&
      (counters = XSyncListSystemCounters(impl->display, &numCounters))) {
    for (int n = 0; n < numCounters; ++n) {
      if (!strcmp(counters[n].name, "SERVERTIME")) {
        impl->serverTimeCounter = counters[n].counter;
        impl->syncSupported     = true;
        break;
      }
    }

    XSyncFreeSystemCounterList(counters);
  }
#else
  (void)impl;
#endif

  return false;
}

PuglWorldInternals*
puglInitWorldInternals(const PuglWorldType type, const PuglWorldFlags flags)
{
  if (type == PUGL_PROGRAM && (flags & PUGL_WORLD_THREADS)) {
    XInitThreads();
  }

  Display* display = XOpenDisplay(NULL);
  if (!display) {
    return NULL;
  }

  PuglWorldInternals* impl =
    (PuglWorldInternals*)calloc(1, sizeof(PuglWorldInternals));

  impl->display = display;

  // Intern the various atoms we will need
  impl->atoms.CLIPBOARD        = XInternAtom(display, "CLIPBOARD", 0);
  impl->atoms.UTF8_STRING      = XInternAtom(display, "UTF8_STRING", 0);
  impl->atoms.WM_PROTOCOLS     = XInternAtom(display, "WM_PROTOCOLS", 0);
  impl->atoms.WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", 0);
  impl->atoms.PUGL_CLIENT_MSG  = XInternAtom(display, "_PUGL_CLIENT_MSG", 0);
  impl->atoms.NET_WM_NAME      = XInternAtom(display, "_NET_WM_NAME", 0);
  impl->atoms.NET_WM_STATE     = XInternAtom(display, "_NET_WM_STATE", 0);
  impl->atoms.NET_WM_STATE_DEMANDS_ATTENTION =
    XInternAtom(display, "_NET_WM_STATE_DEMANDS_ATTENTION", 0);
  impl->atoms.NET_WM_STATE_HIDDEN =
    XInternAtom(display, "_NET_WM_STATE_HIDDEN", 0);

  // Open input method
  XSetLocaleModifiers("");
  if (!(impl->xim = XOpenIM(display, NULL, NULL, NULL))) {
    XSetLocaleModifiers("@im=");
    impl->xim = XOpenIM(display, NULL, NULL, NULL);
  }

  initXSync(impl);
  XFlush(display);

  return impl;
}

void*
puglGetNativeWorld(PuglWorld* const world)
{
  return world->impl->display;
}

PuglInternals*
puglInitViewInternals(void)
{
  PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));

#ifdef HAVE_XCURSOR
  impl->cursorName = cursor_names_x11[0];
#endif

  return impl;
}

static PuglStatus
pollX11Socket(PuglWorld* const world, const double timeout)
{
  if (XPending(world->impl->display) > 0) {
    return PUGL_SUCCESS;
  }

  const int fd   = ConnectionNumber(world->impl->display);
  const int nfds = fd + 1;
  int       ret  = 0;
  fd_set    fds;
  FD_ZERO(&fds); // NOLINT
  FD_SET(fd, &fds);

  if (timeout < 0.0) {
    ret = select(nfds, &fds, NULL, NULL, NULL);
  } else {
    const long     sec  = (long)timeout;
    const long     usec = (long)((timeout - (double)sec) * 1e6);
    struct timeval tv   = {sec, usec};
    ret                 = select(nfds, &fds, NULL, NULL, &tv);
  }

  return ret < 0 ? PUGL_UNKNOWN_ERROR : PUGL_SUCCESS;
}

static PuglView*
findView(PuglWorld* const world, const Window window)
{
  for (size_t i = 0; i < world->numViews; ++i) {
    if (world->views[i]->impl->win == window) {
      return world->views[i];
    }
  }

  return NULL;
}

static PuglStatus
updateSizeHints(const PuglView* const view)
{
  if (!view->impl->win) {
    return PUGL_SUCCESS;
  }

  Display*   display   = view->world->impl->display;
  XSizeHints sizeHints = PUGL_INIT_STRUCT;

  if (!view->hints[PUGL_RESIZABLE]) {
    sizeHints.flags       = PBaseSize | PMinSize | PMaxSize;
    sizeHints.base_width  = (int)view->frame.width;
    sizeHints.base_height = (int)view->frame.height;
    sizeHints.min_width   = (int)view->frame.width;
    sizeHints.min_height  = (int)view->frame.height;
    sizeHints.max_width   = (int)view->frame.width;
    sizeHints.max_height  = (int)view->frame.height;
  } else {
    if (view->defaultWidth || view->defaultHeight) {
      sizeHints.flags |= PBaseSize;
      sizeHints.base_width  = view->defaultWidth;
      sizeHints.base_height = view->defaultHeight;
    }

    if (view->minWidth || view->minHeight) {
      sizeHints.flags |= PMinSize;
      sizeHints.min_width  = view->minWidth;
      sizeHints.min_height = view->minHeight;
    }

    if (view->maxWidth || view->maxHeight) {
      sizeHints.flags |= PMaxSize;
      sizeHints.max_width  = view->maxWidth;
      sizeHints.max_height = view->maxHeight;
    }

    if (view->minAspectX) {
      sizeHints.flags |= PAspect;
      sizeHints.min_aspect.x = view->minAspectX;
      sizeHints.min_aspect.y = view->minAspectY;
      sizeHints.max_aspect.x = view->maxAspectX;
      sizeHints.max_aspect.y = view->maxAspectY;
    }
  }

  XSetNormalHints(display, view->impl->win, &sizeHints);
  return PUGL_SUCCESS;
}

#ifdef HAVE_XCURSOR
static PuglStatus
defineCursorName(PuglView* const view, const char* const name)
{
  PuglInternals* const impl    = view->impl;
  PuglWorld* const     world   = view->world;
  Display* const       display = world->impl->display;
  const Cursor         cur     = XcursorLibraryLoadCursor(display, name);

  if (cur) {
    XDefineCursor(display, impl->win, cur);
    XFreeCursor(display, cur);
    return PUGL_SUCCESS;
  }

  return PUGL_FAILURE;
}
#endif

PuglStatus
puglRealize(PuglView* const view)
{
  PuglInternals* const impl    = view->impl;
  PuglWorld* const     world   = view->world;
  PuglX11Atoms* const  atoms   = &view->world->impl->atoms;
  Display* const       display = world->impl->display;
  const int            screen  = DefaultScreen(display);
  const Window         root    = RootWindow(display, screen);
  const Window         parent  = view->parent ? (Window)view->parent : root;
  XSetWindowAttributes attr    = PUGL_INIT_STRUCT;
  PuglStatus           st      = PUGL_SUCCESS;

  // Ensure that we're unrealized and that a reasonable backend has been set
  if (impl->win) {
    return PUGL_FAILURE;
  }

  if (!view->backend || !view->backend->configure) {
    return PUGL_BAD_BACKEND;
  }

  // Set the size to the default if it has not already been set
  if (view->frame.width <= 0.0 && view->frame.height <= 0.0) {
    if (view->defaultWidth <= 0.0 || view->defaultHeight <= 0.0) {
      return PUGL_BAD_CONFIGURATION;
    }

    view->frame.width  = view->defaultWidth;
    view->frame.height = view->defaultHeight;
  }

  // Center top-level windows if a position has not been set
  if (!view->parent && view->frame.x <= 0.0 && view->frame.y <= 0.0) {
    const int screenWidth  = DisplayWidth(display, screen);
    const int screenHeight = DisplayHeight(display, screen);

    view->frame.x = screenWidth / 2.0 - view->frame.width / 2.0;
    view->frame.y = screenHeight / 2.0 - view->frame.height / 2.0;
  }

  // Configure the backend to get the visual info
  impl->display = display;
  impl->screen  = screen;
  if ((st = view->backend->configure(view)) || !impl->vi) {
    view->backend->destroy(view);
    return st ? st : PUGL_BACKEND_FAILED;
  }

  // Create a colormap based on the visual info from the backend
  attr.colormap = XCreateColormap(display, parent, impl->vi->visual, AllocNone);

  // Set the event mask to request all of the event types we react to
  attr.event_mask |= ButtonPressMask;
  attr.event_mask |= ButtonReleaseMask;
  attr.event_mask |= EnterWindowMask;
  attr.event_mask |= ExposureMask;
  attr.event_mask |= FocusChangeMask;
  attr.event_mask |= KeyPressMask;
  attr.event_mask |= KeyReleaseMask;
  attr.event_mask |= LeaveWindowMask;
  attr.event_mask |= PointerMotionMask;
  attr.event_mask |= PropertyChangeMask;
  attr.event_mask |= StructureNotifyMask;
  attr.event_mask |= VisibilityChangeMask;

  // Create the window
  impl->win = XCreateWindow(display,
                            parent,
                            (int)view->frame.x,
                            (int)view->frame.y,
                            (unsigned)view->frame.width,
                            (unsigned)view->frame.height,
                            0,
                            impl->vi->depth,
                            InputOutput,
                            impl->vi->visual,
                            CWColormap | CWEventMask,
                            &attr);

  // Create the backend drawing context/surface
  if ((st = view->backend->create(view))) {
    return st;
  }

#ifdef HAVE_XRANDR
  int ignored = 0;
  if (XRRQueryExtension(display, &ignored, &ignored)) {
    // Set refresh rate hint to the real refresh rate
    XRRScreenConfiguration* conf         = XRRGetScreenInfo(display, parent);
    short                   current_rate = XRRConfigCurrentRate(conf);

    view->hints[PUGL_REFRESH_RATE] = current_rate;
    XRRFreeScreenConfigInfo(conf);
  }
#endif

  updateSizeHints(view);

  XClassHint classHint = {world->className, world->className};
  XSetClassHint(display, impl->win, &classHint);

  if (view->title) {
    puglSetWindowTitle(view, view->title);
  }

  if (parent == root) {
    XSetWMProtocols(display, impl->win, &atoms->WM_DELETE_WINDOW, 1);
  }

  if (view->transientParent) {
    XSetTransientForHint(display, impl->win, (Window)view->transientParent);
  }

  // Create input context
  impl->xic = XCreateIC(world->impl->xim,
                        XNInputStyle,
                        XIMPreeditNothing | XIMStatusNothing,
                        XNClientWindow,
                        impl->win,
                        XNFocusWindow,
                        impl->win,
                        (XIM)0);

  puglDispatchSimpleEvent(view, PUGL_CREATE);

  return PUGL_SUCCESS;
}

PuglStatus
puglShow(PuglView* const view)
{
  PuglStatus st = view->impl->win ? PUGL_SUCCESS : puglRealize(view);

  if (!st) {
    XMapRaised(view->impl->display, view->impl->win);
    st = puglPostRedisplay(view);
  }

  return st;
}

PuglStatus
puglHide(PuglView* const view)
{
  XUnmapWindow(view->impl->display, view->impl->win);
  return PUGL_SUCCESS;
}

void
puglFreeViewInternals(PuglView* const view)
{
  if (view && view->impl) {
    if (view->impl->xic) {
      XDestroyIC(view->impl->xic);
    }
    if (view->backend) {
      view->backend->destroy(view);
    }
    if (view->impl->display) {
      XDestroyWindow(view->impl->display, view->impl->win);
    }
    XFree(view->impl->vi);
    free(view->impl);
  }
}

void
puglFreeWorldInternals(PuglWorld* const world)
{
  if (world->impl->xim) {
    XCloseIM(world->impl->xim);
  }
  XCloseDisplay(world->impl->display);
  free(world->impl->timers);
  free(world->impl);
}

static PuglKey
keySymToSpecial(const KeySym sym)
{
  // clang-format off
  switch (sym) {
  case XK_F1:               return PUGL_KEY_F1;
  case XK_F2:               return PUGL_KEY_F2;
  case XK_F3:               return PUGL_KEY_F3;
  case XK_F4:               return PUGL_KEY_F4;
  case XK_F5:               return PUGL_KEY_F5;
  case XK_F6:               return PUGL_KEY_F6;
  case XK_F7:               return PUGL_KEY_F7;
  case XK_F8:               return PUGL_KEY_F8;
  case XK_F9:               return PUGL_KEY_F9;
  case XK_F10:              return PUGL_KEY_F10;
  case XK_F11:              return PUGL_KEY_F11;
  case XK_F12:              return PUGL_KEY_F12;
  case XK_Left:             return PUGL_KEY_LEFT;
  case XK_Up:               return PUGL_KEY_UP;
  case XK_Right:            return PUGL_KEY_RIGHT;
  case XK_Down:             return PUGL_KEY_DOWN;
  case XK_Page_Up:          return PUGL_KEY_PAGE_UP;
  case XK_Page_Down:        return PUGL_KEY_PAGE_DOWN;
  case XK_Home:             return PUGL_KEY_HOME;
  case XK_End:              return PUGL_KEY_END;
  case XK_Insert:           return PUGL_KEY_INSERT;
  case XK_Shift_L:          return PUGL_KEY_SHIFT_L;
  case XK_Shift_R:          return PUGL_KEY_SHIFT_R;
  case XK_Control_L:        return PUGL_KEY_CTRL_L;
  case XK_Control_R:        return PUGL_KEY_CTRL_R;
  case XK_Alt_L:            return PUGL_KEY_ALT_L;
  case XK_ISO_Level3_Shift:
  case XK_Alt_R:            return PUGL_KEY_ALT_R;
  case XK_Super_L:          return PUGL_KEY_SUPER_L;
  case XK_Super_R:          return PUGL_KEY_SUPER_R;
  case XK_Menu:             return PUGL_KEY_MENU;
  case XK_Caps_Lock:        return PUGL_KEY_CAPS_LOCK;
  case XK_Scroll_Lock:      return PUGL_KEY_SCROLL_LOCK;
  case XK_Num_Lock:         return PUGL_KEY_NUM_LOCK;
  case XK_Print:            return PUGL_KEY_PRINT_SCREEN;
  case XK_Pause:            return PUGL_KEY_PAUSE;
  default:                  break;
  }
  // clang-format on

  return (PuglKey)0;
}

static int
lookupString(XIC xic, XEvent* const xevent, char* const str, KeySym* const sym)
{
  Status status = 0;

#ifdef X_HAVE_UTF8_STRING
  const int n = Xutf8LookupString(xic, &xevent->xkey, str, 7, sym, &status);
#else
  const int n = XmbLookupString(xic, &xevent->xkey, str, 7, sym, &status);
#endif

  return status == XBufferOverflow ? 0 : n;
}

static void
translateKey(PuglView* const view, XEvent* const xevent, PuglEvent* const event)
{
  const unsigned state  = xevent->xkey.state;
  const bool     filter = XFilterEvent(xevent, None);

  event->key.keycode = xevent->xkey.keycode;
  xevent->xkey.state = 0;

  // Lookup unshifted key
  char          ustr[8] = {0};
  KeySym        sym     = 0;
  const int     ufound  = XLookupString(&xevent->xkey, ustr, 8, &sym, NULL);
  const PuglKey special = keySymToSpecial(sym);

  event->key.key =
    ((special || ufound <= 0) ? special : puglDecodeUTF8((const uint8_t*)ustr));

  if (xevent->type == KeyPress && !filter && !special) {
    // Lookup shifted key for possible text event
    xevent->xkey.state = state;

    char      sstr[8] = {0};
    const int sfound  = lookupString(view->impl->xic, xevent, sstr, &sym);
    if (sfound > 0) {
      // Dispatch key event now
      puglDispatchEvent(view, event);

      // "Return" a text event in its place
      event->text.type      = PUGL_TEXT;
      event->text.character = puglDecodeUTF8((const uint8_t*)sstr);
      memcpy(event->text.string, sstr, sizeof(sstr));
    }
  }
}

static uint32_t
translateModifiers(const unsigned xstate)
{
  return (((xstate & ShiftMask) ? PUGL_MOD_SHIFT : 0u) |
          ((xstate & ControlMask) ? PUGL_MOD_CTRL : 0u) |
          ((xstate & Mod1Mask) ? PUGL_MOD_ALT : 0u) |
          ((xstate & Mod4Mask) ? PUGL_MOD_SUPER : 0u));
}

static PuglStatus
getAtomProperty(PuglView* const view,
                const Window    window,
                const Atom      property,
                unsigned long*  numValues,
                Atom**          values)
{
  Atom          actualType   = 0;
  int           actualFormat = 0;
  unsigned long bytesAfter   = 0;

  return (XGetWindowProperty(view->impl->display,
                             window,
                             property,
                             0,
                             LONG_MAX,
                             False,
                             XA_ATOM,
                             &actualType,
                             &actualFormat,
                             numValues,
                             &bytesAfter,
                             (unsigned char**)values) == Success)
           ? PUGL_SUCCESS
           : PUGL_FAILURE;
}

static PuglEvent
translateClientMessage(PuglView* const view, const XClientMessageEvent message)
{
  const PuglX11Atoms* const atoms = &view->world->impl->atoms;
  PuglEvent                 event = {{PUGL_NOTHING, 0}};

  if (message.message_type == atoms->WM_PROTOCOLS) {
    const Atom protocol = (Atom)message.data.l[0];
    if (protocol == atoms->WM_DELETE_WINDOW) {
      event.type = PUGL_CLOSE;
    }
  } else if (message.message_type == atoms->PUGL_CLIENT_MSG) {
    event.type         = PUGL_CLIENT;
    event.client.data1 = (uintptr_t)message.data.l[0];
    event.client.data2 = (uintptr_t)message.data.l[1];
  }

  return event;
}

static PuglEvent
translatePropertyNotify(PuglView* const view, XPropertyEvent message)
{
  const PuglX11Atoms* const atoms = &view->world->impl->atoms;

  PuglEvent event  = {{PUGL_NOTHING, 0}};
  bool      hidden = false;
  if (message.atom == atoms->NET_WM_STATE) {
    unsigned long numHints = 0;
    Atom*         hints    = NULL;
    if (getAtomProperty(
          view, view->impl->win, message.atom, &numHints, &hints)) {
      return event;
    }

    for (unsigned long i = 0; i < numHints; ++i) {
      if (hints[i] == atoms->NET_WM_STATE_HIDDEN) {
        hidden = true;
      }
    }

    if (hidden && view->visible) {
      event.type = PUGL_UNMAP;
    } else if (!hidden && !view->visible) {
      event.type = PUGL_MAP;
    }

    XFree(hints);
  }

  return event;
}

static PuglEvent
translateEvent(PuglView* const view, XEvent xevent)
{
  PuglEvent event = {{PUGL_NOTHING, 0}};
  event.any.flags = xevent.xany.send_event ? PUGL_IS_SEND_EVENT : 0;

  switch (xevent.type) {
  case ClientMessage:
    event = translateClientMessage(view, xevent.xclient);
    break;
  case PropertyNotify:
    event = translatePropertyNotify(view, xevent.xproperty);
    break;
  case VisibilityNotify:
    if (xevent.xvisibility.state == VisibilityFullyObscured) {
      event.type = PUGL_UNMAP;
    } else {
      event.type = PUGL_MAP;
    }
    break;
  case MapNotify:
    event.type = PUGL_MAP;
    break;
  case UnmapNotify:
    event.type = PUGL_UNMAP;
    break;
  case ConfigureNotify:
    event.type             = PUGL_CONFIGURE;
    event.configure.x      = xevent.xconfigure.x;
    event.configure.y      = xevent.xconfigure.y;
    event.configure.width  = xevent.xconfigure.width;
    event.configure.height = xevent.xconfigure.height;
    break;
  case Expose:
    event.type          = PUGL_EXPOSE;
    event.expose.x      = xevent.xexpose.x;
    event.expose.y      = xevent.xexpose.y;
    event.expose.width  = xevent.xexpose.width;
    event.expose.height = xevent.xexpose.height;
    break;
  case MotionNotify:
    event.type         = PUGL_MOTION;
    event.motion.time  = (double)xevent.xmotion.time / 1e3;
    event.motion.x     = xevent.xmotion.x;
    event.motion.y     = xevent.xmotion.y;
    event.motion.xRoot = xevent.xmotion.x_root;
    event.motion.yRoot = xevent.xmotion.y_root;
    event.motion.state = translateModifiers(xevent.xmotion.state);
    if (xevent.xmotion.is_hint == NotifyHint) {
      event.motion.flags |= PUGL_IS_HINT;
    }
    break;
  case ButtonPress:
  case ButtonRelease:
    if (xevent.type == ButtonPress && xevent.xbutton.button >= 4 &&
        xevent.xbutton.button <= 7) {
      event.type         = PUGL_SCROLL;
      event.scroll.time  = (double)xevent.xbutton.time / 1e3;
      event.scroll.x     = xevent.xbutton.x;
      event.scroll.y     = xevent.xbutton.y;
      event.scroll.xRoot = xevent.xbutton.x_root;
      event.scroll.yRoot = xevent.xbutton.y_root;
      event.scroll.state = translateModifiers(xevent.xbutton.state);
      event.scroll.dx    = 0.0;
      event.scroll.dy    = 0.0;
      switch (xevent.xbutton.button) {
      case 4:
        event.scroll.dy        = 1.0;
        event.scroll.direction = PUGL_SCROLL_UP;
        break;
      case 5:
        event.scroll.dy        = -1.0;
        event.scroll.direction = PUGL_SCROLL_DOWN;
        break;
      case 6:
        event.scroll.dx        = -1.0;
        event.scroll.direction = PUGL_SCROLL_LEFT;
        break;
      case 7:
        event.scroll.dx        = 1.0;
        event.scroll.direction = PUGL_SCROLL_RIGHT;
        break;
      }
    } else if (xevent.xbutton.button < 4 || xevent.xbutton.button > 7) {
      event.button.type   = ((xevent.type == ButtonPress) ? PUGL_BUTTON_PRESS
                                                          : PUGL_BUTTON_RELEASE);
      event.button.time   = (double)xevent.xbutton.time / 1e3;
      event.button.x      = xevent.xbutton.x;
      event.button.y      = xevent.xbutton.y;
      event.button.xRoot  = xevent.xbutton.x_root;
      event.button.yRoot  = xevent.xbutton.y_root;
      event.button.state  = translateModifiers(xevent.xbutton.state);
      event.button.button = xevent.xbutton.button;
    }
    break;
  case KeyPress:
  case KeyRelease:
    event.type =
      ((xevent.type == KeyPress) ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE);
    event.key.time  = (double)xevent.xkey.time / 1e3;
    event.key.x     = xevent.xkey.x;
    event.key.y     = xevent.xkey.y;
    event.key.xRoot = xevent.xkey.x_root;
    event.key.yRoot = xevent.xkey.y_root;
    event.key.state = translateModifiers(xevent.xkey.state);
    translateKey(view, &xevent, &event);
    break;
  case EnterNotify:
  case LeaveNotify:
    event.type =
      ((xevent.type == EnterNotify) ? PUGL_POINTER_IN : PUGL_POINTER_OUT);
    event.crossing.time  = (double)xevent.xcrossing.time / 1e3;
    event.crossing.x     = xevent.xcrossing.x;
    event.crossing.y     = xevent.xcrossing.y;
    event.crossing.xRoot = xevent.xcrossing.x_root;
    event.crossing.yRoot = xevent.xcrossing.y_root;
    event.crossing.state = translateModifiers(xevent.xcrossing.state);
    event.crossing.mode  = PUGL_CROSSING_NORMAL;
    if (xevent.xcrossing.mode == NotifyGrab) {
      event.crossing.mode = PUGL_CROSSING_GRAB;
    } else if (xevent.xcrossing.mode == NotifyUngrab) {
      event.crossing.mode = PUGL_CROSSING_UNGRAB;
    }
    break;

  case FocusIn:
  case FocusOut:
    event.type = (xevent.type == FocusIn) ? PUGL_FOCUS_IN : PUGL_FOCUS_OUT;
    event.focus.mode = PUGL_CROSSING_NORMAL;
    if (xevent.xfocus.mode == NotifyGrab) {
      event.focus.mode = PUGL_CROSSING_GRAB;
    } else if (xevent.xfocus.mode == NotifyUngrab) {
      event.focus.mode = PUGL_CROSSING_UNGRAB;
    }
    break;

  default:
    break;
  }

  return event;
}

PuglStatus
puglGrabFocus(PuglView* const view)
{
  PuglInternals* const impl = view->impl;

  XSetInputFocus(impl->display, impl->win, RevertToNone, CurrentTime);
  return PUGL_SUCCESS;
}

bool
puglHasFocus(const PuglView* const view)
{
  int    revertTo      = 0;
  Window focusedWindow = 0;
  XGetInputFocus(view->impl->display, &focusedWindow, &revertTo);
  return focusedWindow == view->impl->win;
}

PuglStatus
puglRequestAttention(PuglView* const view)
{
  PuglInternals* const      impl  = view->impl;
  const PuglX11Atoms* const atoms = &view->world->impl->atoms;
  XEvent                    event = {0};

  event.type                 = ClientMessage;
  event.xclient.window       = impl->win;
  event.xclient.format       = 32;
  event.xclient.message_type = atoms->NET_WM_STATE;
  event.xclient.data.l[0]    = WM_STATE_ADD;
  event.xclient.data.l[1]    = (long)atoms->NET_WM_STATE_DEMANDS_ATTENTION;
  event.xclient.data.l[2]    = 0;
  event.xclient.data.l[3]    = 1;
  event.xclient.data.l[4]    = 0;

  const Window root = RootWindow(impl->display, impl->screen);
  XSendEvent(impl->display,
             root,
             False,
             SubstructureNotifyMask | SubstructureRedirectMask,
             &event);

  return PUGL_SUCCESS;
}

PuglStatus
puglStartTimer(PuglView* const view, const uintptr_t id, const double timeout)
{
#ifdef HAVE_XSYNC
  if (view->world->impl->syncSupported) {
    XSyncValue value;
    XSyncIntToValue(&value, (int)floor(timeout * 1000.0));

    PuglWorldInternals*  w       = view->world->impl;
    Display* const       display = w->display;
    const XSyncCounter   counter = w->serverTimeCounter;
    const XSyncTestType  type    = XSyncPositiveTransition;
    const XSyncTrigger   trigger = {counter, XSyncRelative, value, type};
    XSyncAlarmAttributes attr    = {trigger, value, True, XSyncAlarmActive};
    const XSyncAlarm     alarm   = XSyncCreateAlarm(display, 0x17, &attr);
    const PuglTimer      timer   = {alarm, view, id};

    if (alarm != None) {
      for (size_t i = 0; i < w->numTimers; ++i) {
        if (w->timers[i].view == view && w->timers[i].id == id) {
          // Replace existing timer
          XSyncDestroyAlarm(w->display, w->timers[i].alarm);
          w->timers[i] = timer;
          return PUGL_SUCCESS;
        }
      }

      // Add new timer
      const size_t size           = ++w->numTimers * sizeof(timer);
      w->timers                   = (PuglTimer*)realloc(w->timers, size);
      w->timers[w->numTimers - 1] = timer;
      return PUGL_SUCCESS;
    }
  }
#else
  (void)view;
  (void)id;
  (void)timeout;
#endif

  return PUGL_FAILURE;
}

PuglStatus
puglStopTimer(PuglView* const view, const uintptr_t id)
{
#ifdef HAVE_XSYNC
  PuglWorldInternals* w = view->world->impl;

  for (size_t i = 0; i < w->numTimers; ++i) {
    if (w->timers[i].view == view && w->timers[i].id == id) {
      XSyncDestroyAlarm(w->display, w->timers[i].alarm);

      if (i == w->numTimers - 1) {
        memset(&w->timers[i], 0, sizeof(PuglTimer));
      } else {
        memmove(w->timers + i,
                w->timers + i + 1,
                sizeof(PuglTimer) * (w->numTimers - i - 1));

        memset(&w->timers[i], 0, sizeof(PuglTimer));
      }

      --w->numTimers;
      return PUGL_SUCCESS;
    }
  }
#else
  (void)view;
  (void)id;
#endif

  return PUGL_FAILURE;
}

static XEvent
eventToX(PuglView* const view, const PuglEvent* const event)
{
  XEvent xev          = {0};
  xev.xany.send_event = True;

  switch (event->type) {
  case PUGL_EXPOSE: {
    const double x = floor(event->expose.x);
    const double y = floor(event->expose.y);
    const double w = ceil(event->expose.x + event->expose.width) - x;
    const double h = ceil(event->expose.y + event->expose.height) - y;

    xev.xexpose.type    = Expose;
    xev.xexpose.serial  = 0;
    xev.xexpose.display = view->impl->display;
    xev.xexpose.window  = view->impl->win;
    xev.xexpose.x       = (int)x;
    xev.xexpose.y       = (int)y;
    xev.xexpose.width   = (int)w;
    xev.xexpose.height  = (int)h;
    break;
  }

  case PUGL_CLIENT:
    xev.xclient.type         = ClientMessage;
    xev.xclient.serial       = 0;
    xev.xclient.send_event   = True;
    xev.xclient.display      = view->impl->display;
    xev.xclient.window       = view->impl->win;
    xev.xclient.message_type = view->world->impl->atoms.PUGL_CLIENT_MSG;
    xev.xclient.format       = 32;
    xev.xclient.data.l[0]    = (long)event->client.data1;
    xev.xclient.data.l[1]    = (long)event->client.data2;
    break;

  default:
    break;
  }

  return xev;
}

PuglStatus
puglSendEvent(PuglView* const view, const PuglEvent* const event)
{
  XEvent xev = eventToX(view, event);

  if (xev.type) {
    if (XSendEvent(view->impl->display, view->impl->win, False, 0, &xev)) {
      return PUGL_SUCCESS;
    }

    return PUGL_UNKNOWN_ERROR;
  }

  return PUGL_UNSUPPORTED_TYPE;
}

#ifndef PUGL_DISABLE_DEPRECATED
PuglStatus
puglWaitForEvent(PuglView* const view)
{
  XEvent xevent;
  XPeekEvent(view->impl->display, &xevent);
  return PUGL_SUCCESS;
}
#endif

static void
mergeExposeEvents(PuglExposeEvent* const dst, const PuglExposeEvent* const src)
{
  if (!dst->type) {
    *dst = *src;
  } else {
    const double max_x = MAX(dst->x + dst->width, src->x + src->width);
    const double max_y = MAX(dst->y + dst->height, src->y + src->height);

    dst->x      = MIN(dst->x, src->x);
    dst->y      = MIN(dst->y, src->y);
    dst->width  = max_x - dst->x;
    dst->height = max_y - dst->y;
  }
}

static void
handleSelectionNotify(const PuglWorld* const world, PuglView* const view)
{
  uint8_t*      str  = NULL;
  Atom          type = 0;
  int           fmt  = 0;
  unsigned long len  = 0;
  unsigned long left = 0;

  XGetWindowProperty(world->impl->display,
                     view->impl->win,
                     XA_PRIMARY,
                     0,
                     0x1FFFFFFF,
                     False,
                     AnyPropertyType,
                     &type,
                     &fmt,
                     &len,
                     &left,
                     &str);

  if (str && fmt == 8 && type == world->impl->atoms.UTF8_STRING && left == 0) {
    puglSetBlob(&view->clipboard, str, len);
  }

  XFree(str);
}

static void
handleSelectionRequest(const PuglWorld* const              world,
                       PuglView* const                     view,
                       const XSelectionRequestEvent* const request)
{
  XSelectionEvent note = {SelectionNotify,
                          request->serial,
                          False,
                          world->impl->display,
                          request->requestor,
                          request->selection,
                          request->target,
                          None,
                          request->time};

  const char* type = NULL;
  size_t      len  = 0;
  const void* data = puglGetInternalClipboard(view, &type, &len);
  if (data && request->selection == world->impl->atoms.CLIPBOARD &&
      request->target == world->impl->atoms.UTF8_STRING) {
    note.property = request->property;
    XChangeProperty(world->impl->display,
                    note.requestor,
                    note.property,
                    note.target,
                    8,
                    PropModeReplace,
                    (const uint8_t*)data,
                    (int)len);
  } else {
    note.property = None;
  }

  XSendEvent(world->impl->display, note.requestor, True, 0, (XEvent*)&note);
}

/// Flush pending configure and expose events for all views
static void
flushExposures(PuglWorld* const world)
{
  for (size_t i = 0; i < world->numViews; ++i) {
    PuglView* const view = world->views[i];

    // Send update event so the application can trigger redraws
    if (view->visible) {
      puglDispatchSimpleEvent(view, PUGL_UPDATE);
    }

    // Copy and reset pending events (in case their handlers write new ones)
    const PuglEvent configure = view->impl->pendingConfigure;
    const PuglEvent expose    = view->impl->pendingExpose;

    view->impl->pendingConfigure.type = PUGL_NOTHING;
    view->impl->pendingExpose.type    = PUGL_NOTHING;

    if (expose.type) {
      view->backend->enter(view, &expose.expose);

      if (configure.type) {
        puglConfigure(view, &configure);
      }

      puglExpose(view, &expose);
      view->backend->leave(view, &expose.expose);
    } else if (configure.type) {
      view->backend->enter(view, NULL);
      puglConfigure(view, &configure);
      view->backend->leave(view, NULL);
    }
  }
}

static bool
handleTimerEvent(PuglWorld* const world, const XEvent xevent)
{
#ifdef HAVE_XSYNC
  if (xevent.type == world->impl->syncEventBase + XSyncAlarmNotify) {
    const XSyncAlarmNotifyEvent* const notify =
      ((const XSyncAlarmNotifyEvent*)&xevent);

    for (size_t i = 0; i < world->impl->numTimers; ++i) {
      if (world->impl->timers[i].alarm == notify->alarm) {
        PuglEvent event = {{PUGL_TIMER, 0}};
        event.timer.id  = world->impl->timers[i].id;
        puglDispatchEvent(world->impl->timers[i].view, &event);
      }
    }

    return true;
  }
#else
  (void)world;
  (void)xevent;
#endif

  return false;
}

static PuglStatus
dispatchX11Events(PuglWorld* const world)
{
  const PuglX11Atoms* const atoms = &world->impl->atoms;

  // Flush output to the server once at the start
  Display* display = world->impl->display;
  XFlush(display);

  // Process all queued events (without further flushing)
  while (XEventsQueued(display, QueuedAfterReading) > 0) {
    XEvent xevent;
    XNextEvent(display, &xevent);

    if (handleTimerEvent(world, xevent)) {
      continue;
    }

    PuglView* view = findView(world, xevent.xany.window);
    if (!view) {
      continue;
    }

    // Handle special events
    PuglInternals* const impl = view->impl;
    if (xevent.type == KeyRelease && view->hints[PUGL_IGNORE_KEY_REPEAT]) {
      XEvent next;
      if (XCheckTypedWindowEvent(display, impl->win, KeyPress, &next) &&
          next.type == KeyPress && next.xkey.time == xevent.xkey.time &&
          next.xkey.keycode == xevent.xkey.keycode) {
        continue;
      }
    } else if (xevent.type == FocusIn) {
      XSetICFocus(impl->xic);
    } else if (xevent.type == FocusOut) {
      XUnsetICFocus(impl->xic);
    } else if (xevent.type == SelectionClear) {
      puglSetBlob(&view->clipboard, NULL, 0);
    } else if (xevent.type == SelectionNotify &&
               xevent.xselection.selection == atoms->CLIPBOARD &&
               xevent.xselection.target == atoms->UTF8_STRING &&
               xevent.xselection.property == XA_PRIMARY) {
      handleSelectionNotify(world, view);
    } else if (xevent.type == SelectionRequest) {
      handleSelectionRequest(world, view, &xevent.xselectionrequest);
    }

    // Translate X11 event to Pugl event
    const PuglEvent event = translateEvent(view, xevent);

    if (event.type == PUGL_EXPOSE) {
      // Expand expose event to be dispatched after loop
      mergeExposeEvents(&view->impl->pendingExpose.expose, &event.expose);
    } else if (event.type == PUGL_CONFIGURE) {
      // Update configure event to be dispatched after loop
      view->impl->pendingConfigure = event;
    } else if (event.type == PUGL_MAP) {
      // Get initial window position and size
      XWindowAttributes attrs;
      XGetWindowAttributes(view->impl->display, view->impl->win, &attrs);

      // Build an initial configure event in case the WM doesn't send one
      PuglEvent configureEvent        = {{PUGL_CONFIGURE, 0}};
      configureEvent.configure.x      = (double)attrs.x;
      configureEvent.configure.y      = (double)attrs.y;
      configureEvent.configure.width  = (double)attrs.width;
      configureEvent.configure.height = (double)attrs.height;

      // Dispatch an initial configure (if necessary), then the map event
      puglDispatchEvent(view, &configureEvent);
      puglDispatchEvent(view, &event);
    } else {
      // Dispatch event to application immediately
      puglDispatchEvent(view, &event);
    }
  }

  return PUGL_SUCCESS;
}

#ifndef PUGL_DISABLE_DEPRECATED
PuglStatus
puglProcessEvents(PuglView* const view)
{
  return puglUpdate(view->world, 0.0);
}
#endif

PuglStatus
puglUpdate(PuglWorld* const world, const double timeout)
{
  const double startTime = puglGetTime(world);
  PuglStatus   st        = PUGL_SUCCESS;

  world->impl->dispatchingEvents = true;

  if (timeout < 0.0) {
    st = pollX11Socket(world, timeout);
    st = st ? st : dispatchX11Events(world);
  } else if (timeout <= 0.001) {
    st = dispatchX11Events(world);
  } else {
    const double endTime = startTime + timeout - 0.001;
    double       t       = startTime;
    while (!st && t < endTime) {
      if (!(st = pollX11Socket(world, endTime - t))) {
        st = dispatchX11Events(world);
      }

      t = puglGetTime(world);
    }
  }

  flushExposures(world);

  world->impl->dispatchingEvents = false;

  return st;
}

double
puglGetTime(const PuglWorld* const world)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0) -
         world->startTime;
}

PuglStatus
puglPostRedisplay(PuglView* const view)
{
  const PuglRect rect = {0, 0, view->frame.width, view->frame.height};

  return puglPostRedisplayRect(view, rect);
}

PuglStatus
puglPostRedisplayRect(PuglView* const view, const PuglRect rect)
{
  const PuglExposeEvent event = {
    PUGL_EXPOSE, 0, rect.x, rect.y, rect.width, rect.height};

  if (view->world->impl->dispatchingEvents) {
    // Currently dispatching events, add/expand expose for the loop end
    mergeExposeEvents(&view->impl->pendingExpose.expose, &event);
  } else if (view->visible) {
    // Not dispatching events, send an X expose so we wake up next time
    PuglEvent exposeEvent = {{PUGL_EXPOSE, 0}};
    exposeEvent.expose    = event;
    return puglSendEvent(view, &exposeEvent);
  }

  return PUGL_SUCCESS;
}

PuglNativeView
puglGetNativeWindow(PuglView* const view)
{
  return (PuglNativeView)view->impl->win;
}

PuglStatus
puglSetWindowTitle(PuglView* const view, const char* const title)
{
  Display*                  display = view->world->impl->display;
  const PuglX11Atoms* const atoms   = &view->world->impl->atoms;

  puglSetString(&view->title, title);

  if (view->impl->win) {
    XStoreName(display, view->impl->win, title);
    XChangeProperty(display,
                    view->impl->win,
                    atoms->NET_WM_NAME,
                    atoms->UTF8_STRING,
                    8,
                    PropModeReplace,
                    (const uint8_t*)title,
                    (int)strlen(title));
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglSetFrame(PuglView* const view, const PuglRect frame)
{
  if (view->impl->win) {
    if (!XMoveResizeWindow(view->world->impl->display,
                           view->impl->win,
                           (int)frame.x,
                           (int)frame.y,
                           (unsigned)frame.width,
                           (unsigned)frame.height)) {
      return PUGL_UNKNOWN_ERROR;
    }
  }

  view->frame = frame;
  return PUGL_SUCCESS;
}

PuglStatus
puglSetDefaultSize(PuglView* const view, const int width, const int height)
{
  view->defaultWidth  = width;
  view->defaultHeight = height;
  return updateSizeHints(view);
}

PuglStatus
puglSetMinSize(PuglView* const view, const int width, const int height)
{
  view->minWidth  = width;
  view->minHeight = height;
  return updateSizeHints(view);
}

PuglStatus
puglSetMaxSize(PuglView* const view, const int width, const int height)
{
  view->maxWidth  = width;
  view->maxHeight = height;
  return updateSizeHints(view);
}

PuglStatus
puglSetAspectRatio(PuglView* const view,
                   const int       minX,
                   const int       minY,
                   const int       maxX,
                   const int       maxY)
{
  view->minAspectX = minX;
  view->minAspectY = minY;
  view->maxAspectX = maxX;
  view->maxAspectY = maxY;
  return updateSizeHints(view);
}

PuglStatus
puglSetTransientFor(PuglView* const view, const PuglNativeView parent)
{
  Display* display = view->world->impl->display;

  view->transientParent = parent;

  if (view->impl->win) {
    XSetTransientForHint(
      display, view->impl->win, (Window)view->transientParent);
  }

  return PUGL_SUCCESS;
}

const void*
puglGetClipboard(PuglView* const    view,
                 const char** const type,
                 size_t* const      len)
{
  PuglInternals* const      impl  = view->impl;
  const PuglX11Atoms* const atoms = &view->world->impl->atoms;

  const Window owner = XGetSelectionOwner(impl->display, atoms->CLIPBOARD);
  if (owner != None && owner != impl->win) {
    // Clear internal selection
    puglSetBlob(&view->clipboard, NULL, 0);

    // Request selection from the owner
    XConvertSelection(impl->display,
                      atoms->CLIPBOARD,
                      atoms->UTF8_STRING,
                      XA_PRIMARY,
                      impl->win,
                      CurrentTime);

    // Run event loop until data is received, within limits
    for (int i=500; !view->clipboard.data && --i >= 0;) {
      puglUpdate(view->world, -1.0);
    }
  }

  return puglGetInternalClipboard(view, type, len);
}

PuglStatus
puglSetClipboard(PuglView* const   view,
                 const char* const type,
                 const void* const data,
                 const size_t      len)
{
  PuglInternals* const      impl  = view->impl;
  const PuglX11Atoms* const atoms = &view->world->impl->atoms;

  PuglStatus st = puglSetInternalClipboard(view, type, data, len);
  if (!st) {
    XSetSelectionOwner(impl->display, atoms->CLIPBOARD, impl->win, CurrentTime);
  }

  return st;
}

PuglStatus
puglSetCursor(PuglView* const view, const PuglCursor cursor)
{
#ifdef HAVE_XCURSOR
  PuglInternals* const impl  = view->impl;
  const unsigned       index = (unsigned)cursor;
  const unsigned       count = sizeof(cursor_names_x11) / sizeof(cursor_names_x11[0]);
  if (index >= count) {
    return PUGL_BAD_PARAMETER;
  }

  const char* name = cursor_names_x11[index];
  if (!impl->win || impl->cursorName == name) {
    return PUGL_SUCCESS;
  }

  impl->cursorName = cursor_names_x11[index];

  return defineCursorName(view, impl->cursorName);
#else
  (void)view;
  (void)cursor;
  return PUGL_FAILURE;
#endif
}

// Semi-public platform API used by backends

PuglStatus
puglX11Configure(PuglView* view)
{
  PuglInternals* const impl = view->impl;
  XVisualInfo          pat  = PUGL_INIT_STRUCT;
  int                  n    = 0;

  pat.screen = impl->screen;
  impl->vi   = XGetVisualInfo(impl->display, VisualScreenMask, &pat, &n);

  view->hints[PUGL_RED_BITS]   = impl->vi->bits_per_rgb;
  view->hints[PUGL_GREEN_BITS] = impl->vi->bits_per_rgb;
  view->hints[PUGL_BLUE_BITS]  = impl->vi->bits_per_rgb;
  view->hints[PUGL_ALPHA_BITS] = 0;

  return PUGL_SUCCESS;
}
