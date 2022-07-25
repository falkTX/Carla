// Copyright 2012-2022 David Robillard <d@drobilla.net>
// Copyright 2013 Robin Gareus <robin@gareus.org>
// Copyright 2011-2012 Ben Loftis, Harrison Consoles
// SPDX-License-Identifier: ISC

#include "x11.h"

#include "attributes.h"
#include "internal.h"
#include "platform.h"
#include "types.h"

#include "pugl/pugl.h"

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
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
#endif

#include <sys/select.h>

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

#define NUM_CURSORS ((unsigned)PUGL_CURSOR_ANTI_DIAGONAL + 1u)

#ifdef HAVE_XCURSOR
static const char* const cursor_names[NUM_CURSORS] = {
  "default",           // ARROW
  "text",              // CARET
  "crosshair",         // CROSSHAIR
  "pointer",           // HAND
  "not-allowed",       // NO
  "sb_h_double_arrow", // LEFT_RIGHT
  "sb_v_double_arrow", // UP_DOWN
  "size_fdiag",        // DIAGONAL
  "size_bdiag"         // ANTI_DIAGONAL
};
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

static double
puglX11GetDisplayScaleFactor(Display* const display)
{
  double            dpi = 96.0;
  const char* const rms = XResourceManagerString(display);
  if (rms) {
    XrmDatabase db = XrmGetStringDatabase(rms);
    if (db) {
      XrmValue value = {0u, NULL};
      char*    type  = NULL;
      if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value)) {
        if (!type || !strcmp(type, "String")) {
          char*        end    = NULL;
          const double xftDpi = strtod(value.addr, &end);
          if (xftDpi > 0.0 && xftDpi < HUGE_VAL) {
            dpi = xftDpi;
          }
        }
      }

      XrmDestroyDatabase(db);
    }
  }

  return dpi / 96.0;
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

  impl->display     = display;
  impl->scaleFactor = puglX11GetDisplayScaleFactor(display);

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

  impl->atoms.TARGETS       = XInternAtom(display, "TARGETS", 0);
  impl->atoms.text_uri_list = XInternAtom(display, "text/uri-list", 0);

  // Open input method
  XSetLocaleModifiers("");
  if (!(impl->xim = XOpenIM(display, NULL, NULL, NULL))) {
    XSetLocaleModifiers("@im=");
    impl->xim = XOpenIM(display, NULL, NULL, NULL);
  }

  XrmInitialize();
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
puglInitViewInternals(PuglWorld* const world)
{
  PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));

  impl->clipboard.selection = world->impl->atoms.CLIPBOARD;
  impl->clipboard.property  = XA_PRIMARY;

#ifdef HAVE_XCURSOR
  impl->cursorName = cursor_names[PUGL_CURSOR_ARROW];
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
    const PuglViewSize defaultSize = view->sizeHints[PUGL_DEFAULT_SIZE];
    if (defaultSize.width && defaultSize.height) {
      sizeHints.flags |= PBaseSize;
      sizeHints.base_width  = defaultSize.width;
      sizeHints.base_height = defaultSize.height;
    }

    const PuglViewSize minSize = view->sizeHints[PUGL_MIN_SIZE];
    if (minSize.width && minSize.height) {
      sizeHints.flags |= PMinSize;
      sizeHints.min_width  = minSize.width;
      sizeHints.min_height = minSize.height;
    }

    const PuglViewSize maxSize = view->sizeHints[PUGL_MAX_SIZE];
    if (maxSize.width && maxSize.height) {
      sizeHints.flags |= PMaxSize;
      sizeHints.max_width  = maxSize.width;
      sizeHints.max_height = maxSize.height;
    }

    const PuglViewSize minAspect = view->sizeHints[PUGL_MIN_ASPECT];
    const PuglViewSize maxAspect = view->sizeHints[PUGL_MAX_ASPECT];
    if (minAspect.width && minAspect.height && maxAspect.width &&
        maxAspect.height) {
      sizeHints.flags |= PAspect;
      sizeHints.min_aspect.x = minAspect.width;
      sizeHints.min_aspect.y = minAspect.height;
      sizeHints.max_aspect.x = maxAspect.width;
      sizeHints.max_aspect.y = maxAspect.height;
    }

    const PuglViewSize fixedAspect = view->sizeHints[PUGL_FIXED_ASPECT];
    if (fixedAspect.width && fixedAspect.height) {
      sizeHints.flags |= PAspect;
      sizeHints.min_aspect.x = fixedAspect.width;
      sizeHints.min_aspect.y = fixedAspect.height;
      sizeHints.max_aspect.x = fixedAspect.width;
      sizeHints.max_aspect.y = fixedAspect.height;
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

  // Load cursor theme
  char* const theme = XcursorGetTheme(display);
  if (!theme) {
    return PUGL_FAILURE;
  }

  // Get the default size and cursor image from it
  const int           size  = XcursorGetDefaultSize(display);
  XcursorImage* const image = XcursorLibraryLoadImage(name, theme, size);
  if (!image) {
    return PUGL_BAD_PARAMETER;
  }

  // Load a cursor from the image
  const Cursor cur = XcursorImageLoadCursor(display, image);
  XcursorImageDestroy(image);
  if (!cur) {
    return PUGL_UNKNOWN_ERROR;
  }

  // Set the view's cursor to the new loaded one
  XDefineCursor(display, impl->win, cur);
  XFreeCursor(display, cur);
  return PUGL_SUCCESS;
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
    const PuglViewSize defaultSize = view->sizeHints[PUGL_DEFAULT_SIZE];
    if (!defaultSize.width || !defaultSize.height) {
      return PUGL_BAD_CONFIGURATION;
    }

    view->frame.width  = defaultSize.width;
    view->frame.height = defaultSize.height;
  }

  // Center top-level windows if a position has not been set
  if (!view->parent && !view->frame.x && !view->frame.y) {
    const int screenWidth  = DisplayWidth(display, screen);
    const int screenHeight = DisplayHeight(display, screen);

    view->frame.x = (PuglCoord)((screenWidth - view->frame.width) / 2);
    view->frame.y = (PuglCoord)((screenHeight - view->frame.height) / 2);
  }

  // Configure the backend to get the visual info
  impl->screen = screen;
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
                            view->frame.x,
                            view->frame.y,
                            view->frame.width,
                            view->frame.height,
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
  if (world->impl->xim) {
    impl->xic = XCreateIC(world->impl->xim,
                          XNInputStyle,
                          XIMPreeditNothing | XIMStatusNothing,
                          XNClientWindow,
                          impl->win,
                          XNFocusWindow,
                          impl->win,
                          (XIM)0);
  }

  puglDispatchSimpleEvent(view, PUGL_CREATE);

  /* Flush before returning for two reasons: so that hints are available to the
     view's parent via the X server during embedding, and so that the X server
     has a chance to create the window (and make its actual position and size
     known) before any children are created.  Potential bugs aside, this
     increases the chances that an application will be cleanly configured once
     on startup with the correct position and size. */
  XFlush(display);

  return PUGL_SUCCESS;
}

PuglStatus
puglShow(PuglView* const view)
{
  PuglStatus st = view->impl->win ? PUGL_SUCCESS : puglRealize(view);

  if (!st) {
    XMapRaised(view->world->impl->display, view->impl->win);
    st = puglPostRedisplay(view);
  }

  return st;
}

PuglStatus
puglHide(PuglView* const view)
{
  XUnmapWindow(view->world->impl->display, view->impl->win);
  return PUGL_SUCCESS;
}

static void
clearX11Clipboard(PuglX11Clipboard* const board)
{
  for (unsigned long i = 0; i < board->numFormats; ++i) {
    free(board->formatStrings[i]);
    board->formatStrings[i] = NULL;
  }

  board->source              = None;
  board->numFormats          = 0;
  board->acceptedFormatIndex = UINT32_MAX;
  board->acceptedFormat      = None;
  board->data.len            = 0;
}

void
puglFreeViewInternals(PuglView* const view)
{
  if (view && view->impl) {
    clearX11Clipboard(&view->impl->clipboard);
    free(view->impl->clipboard.data.data);
    free(view->impl->clipboard.formats);
    free(view->impl->clipboard.formatStrings);
    if (view->impl->xic) {
      XDestroyIC(view->impl->xic);
    }
    if (view->backend) {
      view->backend->destroy(view);
    }
    if (view->world->impl->display && view->impl->win) {
      XDestroyWindow(view->world->impl->display, view->impl->win);
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

  if (xevent->type == KeyPress && !filter && !special && view->impl->xic) {
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

  return (XGetWindowProperty(view->world->impl->display,
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

static PuglX11Clipboard*
getX11SelectionClipboard(PuglView* const view, const Atom selection)
{
  return (selection == view->world->impl->atoms.CLIPBOARD)
           ? &view->impl->clipboard
           : NULL;
}

static void
setClipboardFormats(PuglView* const         view,
                    PuglX11Clipboard* const board,
                    const unsigned long     numFormats,
                    const Atom* const       formats)
{
  Atom* const newFormats =
    (Atom*)realloc(board->formats, numFormats * sizeof(Atom));
  if (!newFormats) {
    return;
  }

  for (unsigned long i = 0; i < board->numFormats; ++i) {
    free(board->formatStrings[i]);
    board->formatStrings[i] = NULL;
  }

  board->formats    = newFormats;
  board->numFormats = 0;

  board->formatStrings =
    (char**)realloc(board->formatStrings, numFormats * sizeof(char*));

  for (unsigned long i = 0; i < numFormats; ++i) {
    if (formats[i]) {
      char* const name = XGetAtomName(view->world->impl->display, formats[i]);
      const char* type = NULL;

      if (strchr(name, '/')) { // MIME type (hopefully)
        type = name;
      } else if (!strcmp(name, "UTF8_STRING")) { // Plain text
        type = "text/plain";
      }

      if (type) {
        const size_t typeLen      = strlen(type);
        char* const  formatString = (char*)calloc(typeLen + 1, 1);

        memcpy(formatString, type, typeLen + 1);

        board->formats[board->numFormats]       = formats[i];
        board->formatStrings[board->numFormats] = formatString;
        ++board->numFormats;
      }

      XFree(name);
    }
  }
}

static PuglEvent
translateClientMessage(PuglView* const view, XClientMessageEvent message)
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

  PuglEvent event = {{PUGL_NOTHING, 0}};
  if (message.atom == atoms->NET_WM_STATE) {
    unsigned long numHints = 0;
    Atom*         hints    = NULL;
    if (getAtomProperty(
          view, view->impl->win, message.atom, &numHints, &hints)) {
      return event;
    }

    bool hidden = false;
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
    event.type = (xevent.xvisibility.state == VisibilityFullyObscured)
                   ? PUGL_UNMAP
                   : PUGL_MAP;
    break;
  case MapNotify:
    event.type = PUGL_MAP;
    break;
  case UnmapNotify:
    event.type = PUGL_UNMAP;
    break;
  case ConfigureNotify:
    event.type             = PUGL_CONFIGURE;
    event.configure.x      = (PuglCoord)xevent.xconfigure.x;
    event.configure.y      = (PuglCoord)xevent.xconfigure.y;
    event.configure.width  = (PuglSpan)xevent.xconfigure.width;
    event.configure.height = (PuglSpan)xevent.xconfigure.height;
    break;
  case Expose:
    event.type          = PUGL_EXPOSE;
    event.expose.x      = (PuglCoord)xevent.xexpose.x;
    event.expose.y      = (PuglCoord)xevent.xexpose.y;
    event.expose.width  = (PuglSpan)xevent.xexpose.width;
    event.expose.height = (PuglSpan)xevent.xexpose.height;
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
      event.button.button = xevent.xbutton.button - 1;
      if (event.button.button == 1) {
        event.button.button = 2;
      } else if (event.button.button == 2) {
        event.button.button = 1;
      } else if (event.button.button >= 7) {
        event.button.button -= 4;
      }
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
  PuglInternals* const impl    = view->impl;
  Display* const       display = view->world->impl->display;
  XWindowAttributes    attrs   = PUGL_INIT_STRUCT;

  if (!impl->win || !XGetWindowAttributes(display, impl->win, &attrs)) {
    return PUGL_UNKNOWN_ERROR;
  }

  if (attrs.map_state != IsViewable) {
    return PUGL_FAILURE;
  }

  XSetInputFocus(display, impl->win, RevertToNone, CurrentTime);
  return PUGL_SUCCESS;
}

bool
puglHasFocus(const PuglView* const view)
{
  int    revertTo      = 0;
  Window focusedWindow = 0;
  XGetInputFocus(view->world->impl->display, &focusedWindow, &revertTo);
  return focusedWindow == view->impl->win;
}

PuglStatus
puglRequestAttention(PuglView* const view)
{
  PuglInternals* const      impl    = view->impl;
  Display* const            display = view->world->impl->display;
  const PuglX11Atoms* const atoms   = &view->world->impl->atoms;
  XEvent                    event   = PUGL_INIT_STRUCT;

  event.type                 = ClientMessage;
  event.xclient.window       = impl->win;
  event.xclient.format       = 32;
  event.xclient.message_type = atoms->NET_WM_STATE;
  event.xclient.data.l[0]    = WM_STATE_ADD;
  event.xclient.data.l[1]    = (long)atoms->NET_WM_STATE_DEMANDS_ATTENTION;
  event.xclient.data.l[2]    = 0;
  event.xclient.data.l[3]    = 1;
  event.xclient.data.l[4]    = 0;

  const Window root = RootWindow(display, impl->screen);

  return XSendEvent(display,
                    root,
                    False,
                    SubstructureNotifyMask | SubstructureRedirectMask,
                    &event)
           ? PUGL_SUCCESS
           : PUGL_UNKNOWN_ERROR;
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
  XEvent xev          = PUGL_INIT_STRUCT;
  xev.xany.send_event = True;

  switch (event->type) {
  case PUGL_EXPOSE: {
    const double x = floor(event->expose.x);
    const double y = floor(event->expose.y);
    const double w = ceil(event->expose.x + event->expose.width) - x;
    const double h = ceil(event->expose.y + event->expose.height) - y;

    xev.xexpose.type    = Expose;
    xev.xexpose.serial  = 0;
    xev.xexpose.display = view->world->impl->display;
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
    xev.xclient.display      = view->world->impl->display;
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
    return XSendEvent(
             view->world->impl->display, view->impl->win, False, 0, &xev)
             ? PUGL_SUCCESS
             : PUGL_UNKNOWN_ERROR;
  }

  return PUGL_UNSUPPORTED;
}

#ifndef PUGL_DISABLE_DEPRECATED
PuglStatus
puglWaitForEvent(PuglView* const view)
{
  XEvent xevent;
  XPeekEvent(view->world->impl->display, &xevent);
  return PUGL_SUCCESS;
}
#endif

static void
mergeExposeEvents(PuglExposeEvent* const dst, const PuglExposeEvent* const src)
{
  if (!dst->type) {
    *dst = *src;
  } else {
    const int dst_r = dst->x + dst->width;
    const int src_r = src->x + src->width;
    const int max_x = MAX(dst_r, src_r);
    const int dst_b = dst->y + dst->height;
    const int src_b = src->y + src->height;
    const int max_y = MAX(dst_b, src_b);

    dst->x      = (PuglCoord)MIN(dst->x, src->x);
    dst->y      = (PuglCoord)MIN(dst->y, src->y);
    dst->width  = (PuglSpan)(max_x - dst->x);
    dst->height = (PuglSpan)(max_y - dst->y);
  }
}

static PuglStatus
retrieveSelection(const PuglWorld* const world,
                  PuglView* const        view,
                  const Atom             property,
                  const Atom             type,
                  PuglBlob* const        result)
{
  uint8_t*      value          = NULL;
  Atom          actualType     = 0u;
  int           actualFormat   = 0;
  unsigned long actualNumItems = 0u;
  unsigned long bytesAfter     = 0u;

  if (XGetWindowProperty(world->impl->display,
                         view->impl->win,
                         property,
                         0,
                         0x1FFFFFFF,
                         False,
                         type,
                         &actualType,
                         &actualFormat,
                         &actualNumItems,
                         &bytesAfter,
                         &value) != Success) {
    return PUGL_FAILURE;
  }

  if (value && actualFormat == 8 && bytesAfter == 0) {
    puglSetBlob(result, value, actualNumItems);
  }

  XFree(value);
  return PUGL_SUCCESS;
}

static void
handleSelectionNotify(const PuglWorld* const       world,
                      PuglView* const              view,
                      const XSelectionEvent* const event)
{
  const PuglX11Atoms* const atoms = &world->impl->atoms;

  Display* const          display   = view->world->impl->display;
  const Atom              selection = event->selection;
  PuglX11Clipboard* const board     = getX11SelectionClipboard(view, selection);
  PuglEvent               puglEvent = {{PUGL_NOTHING, 0}};

  if (event->target == atoms->TARGETS) {
    // Notification of available datatypes
    unsigned long numFormats = 0;
    Atom*         formats    = NULL;
    if (!getAtomProperty(
          view, event->requestor, event->property, &numFormats, &formats)) {
      setClipboardFormats(view, board, numFormats, formats);

      const PuglDataOfferEvent offer = {
        PUGL_DATA_OFFER, 0, (double)event->time / 1e3};

      puglEvent.offer            = offer;
      board->acceptedFormatIndex = UINT32_MAX;
      board->acceptedFormat      = None;

      XFree(formats);
    }

  } else if (event->selection == atoms->CLIPBOARD &&
             event->property == XA_PRIMARY &&
             board->acceptedFormatIndex < board->numFormats) {
    // Notification of data from the clipboard
    if (!retrieveSelection(
          world, view, event->property, event->target, &board->data)) {
      board->source = XGetSelectionOwner(display, board->selection);

      const PuglDataEvent data = {
        PUGL_DATA, 0u, (double)event->time / 1e3, board->acceptedFormatIndex};

      puglEvent.data = data;
    }
  }

  puglDispatchEvent(view, &puglEvent);
}

static PuglStatus
handleSelectionRequest(const PuglWorld* const              world,
                       PuglView* const                     view,
                       const XSelectionRequestEvent* const request)
{
  Display* const            display = world->impl->display;
  const PuglX11Atoms* const atoms   = &world->impl->atoms;

  PuglX11Clipboard* const board =
    getX11SelectionClipboard(view, request->selection);

  if (!board) {
    return PUGL_UNKNOWN_ERROR;
  }

  if (request->target == atoms->TARGETS) {
    XChangeProperty(world->impl->display,
                    request->requestor,
                    request->property,
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    (const uint8_t*)board->formats,
                    (int)board->numFormats);
  } else {
    XChangeProperty(world->impl->display,
                    request->requestor,
                    request->property,
                    request->target,
                    8,
                    PropModeReplace,
                    (const uint8_t*)board->data.data,
                    (int)board->data.len);
  }

  XSelectionEvent note = {SelectionNotify,
                          request->serial,
                          False,
                          display,
                          request->requestor,
                          request->selection,
                          request->target,
                          request->property,
                          request->time};

  return XSendEvent(
           world->impl->display, note.requestor, True, 0, (XEvent*)&note)
           ? PUGL_SUCCESS
           : PUGL_UNKNOWN_ERROR;
}

/// Flush pending configure and expose events for all views
PUGL_WARN_UNUSED_RESULT
static PuglStatus
flushExposures(PuglWorld* const world)
{
  PuglStatus st0 = PUGL_SUCCESS;
  PuglStatus st1 = PUGL_SUCCESS;
  PuglStatus st2 = PUGL_SUCCESS;

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
      if (!(st0 = view->backend->enter(view, &expose.expose))) {
        if (configure.type) {
          st0 = puglConfigure(view, &configure);
        }

        st1 = puglExpose(view, &expose);
        st2 = view->backend->leave(view, &expose.expose);
      }
    } else if (configure.type) {
      if (!(st0 = view->backend->enter(view, NULL))) {
        st0 = puglConfigure(view, &configure);
        st1 = view->backend->leave(view, NULL);
      }
    }
  }

  return st0 ? st0 : st1 ? st1 : st2;
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
dispatchCurrentConfiguration(PuglView* const view)
{
  // Get initial window position and size
  XWindowAttributes attrs;
  XGetWindowAttributes(view->world->impl->display, view->impl->win, &attrs);

  // Build an initial configure event in case the WM doesn't send one
  PuglEvent configureEvent        = {{PUGL_CONFIGURE, 0}};
  configureEvent.configure.x      = (PuglCoord)attrs.x;
  configureEvent.configure.y      = (PuglCoord)attrs.y;
  configureEvent.configure.width  = (PuglSpan)attrs.width;
  configureEvent.configure.height = (PuglSpan)attrs.height;

  return puglDispatchEvent(view, &configureEvent);
}

static PuglStatus
dispatchX11Events(PuglWorld* const world)
{
  PuglStatus st0 = PUGL_SUCCESS;
  PuglStatus st1 = PUGL_SUCCESS;

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

    PuglView* const view = findView(world, xevent.xany.window);
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
    } else if (xevent.type == SelectionClear) {
      PuglX11Clipboard* const board =
        getX11SelectionClipboard(view, xevent.xselectionclear.selection);
      if (board) {
        clearX11Clipboard(board);
      }
    } else if (xevent.type == SelectionNotify) {
      handleSelectionNotify(world, view, &xevent.xselection);
    } else if (xevent.type == SelectionRequest) {
      handleSelectionRequest(world, view, &xevent.xselectionrequest);
    }

    // Translate X11 event to Pugl event
    const PuglEvent event = translateEvent(view, xevent);

    switch (event.type) {
    case PUGL_CONFIGURE:
      // Update configure event to be dispatched after loop
      view->impl->pendingConfigure = event;
      break;
    case PUGL_MAP:
      // Dispatch an initial configure (if necessary), then the map event
      st0 = dispatchCurrentConfiguration(view);
      st1 = puglDispatchEvent(view, &event);
      break;
    case PUGL_EXPOSE:
      // Expand expose event to be dispatched after loop
      mergeExposeEvents(&view->impl->pendingExpose.expose, &event.expose);
      break;
    case PUGL_FOCUS_IN:
      // Set the input context focus
      if (view->impl->xic) {
        XSetICFocus(view->impl->xic);
      }
      break;
    case PUGL_FOCUS_OUT:
      // Unset the input context focus
      if (view->impl->xic) {
        XUnsetICFocus(view->impl->xic);
      }
      break;
    default:
      // Dispatch event to application immediately
      st0 = puglDispatchEvent(view, &event);
      break;
    }
  }

  return st0 ? st0 : st1;
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
  PuglStatus   st0       = PUGL_SUCCESS;
  PuglStatus   st1       = PUGL_SUCCESS;

  world->impl->dispatchingEvents = true;

  if (timeout < 0.0) {
    st0 = pollX11Socket(world, timeout);
    st0 = st0 ? st0 : dispatchX11Events(world);
  } else if (timeout <= 0.001) {
    st0 = dispatchX11Events(world);
  } else {
    const double endTime = startTime + timeout - 0.001;
    double       t       = startTime;
    while (!st0 && t < endTime) {
      if (!(st0 = pollX11Socket(world, endTime - t))) {
        st0 = dispatchX11Events(world);
      }

      t = puglGetTime(world);
    }
  }

  st1 = flushExposures(world);

  world->impl->dispatchingEvents = false;

  return st0 ? st0 : st1;
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
puglGetNativeView(PuglView* const view)
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

double
puglGetScaleFactor(const PuglView* const view)
{
  return view->world->impl->scaleFactor;
}

PuglStatus
puglSetFrame(PuglView* const view, const PuglRect frame)
{
  if (view->impl->win) {
    if (!XMoveResizeWindow(view->world->impl->display,
                           view->impl->win,
                           frame.x,
                           frame.y,
                           frame.width,
                           frame.height)) {
      return PUGL_UNKNOWN_ERROR;
    }
  }

  view->frame = frame;
  return PUGL_SUCCESS;
}

PuglStatus
puglSetPosition(PuglView* const view, const int x, const int y)
{
  Display* const display = view->world->impl->display;
  const Window   win     = view->impl->win;

  if (x > INT16_MAX || y > INT16_MAX) {
    return PUGL_BAD_PARAMETER;
  }

  if (win && !XMoveWindow(display, win, x, y)) {
    return PUGL_UNKNOWN_ERROR;
  }

  view->frame.x = (PuglCoord)x;
  view->frame.y = (PuglCoord)y;
  return PUGL_SUCCESS;
}

PuglStatus
puglSetSize(PuglView* const view, const unsigned width, const unsigned height)
{
  Display* const display = view->world->impl->display;
  const Window   win     = view->impl->win;

  if (width > INT16_MAX || height > INT16_MAX) {
    return PUGL_BAD_PARAMETER;
  }

  if (win) {
    return XResizeWindow(display, win, width, height) ? PUGL_SUCCESS
                                                      : PUGL_UNKNOWN_ERROR;
  }

  view->frame.width  = (PuglSpan)width;
  view->frame.height = (PuglSpan)height;
  return PUGL_SUCCESS;
}

PuglStatus
puglSetSizeHint(PuglView* const    view,
                const PuglSizeHint hint,
                const PuglSpan     width,
                const PuglSpan     height)
{
  view->sizeHints[hint].width  = width;
  view->sizeHints[hint].height = height;
  return updateSizeHints(view);
}

PuglStatus
puglSetTransientParent(PuglView* const view, const PuglNativeView parent)
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
puglGetClipboard(PuglView* const view,
                 const uint32_t  typeIndex,
                 size_t* const   len)
{
  Display* const          display = view->world->impl->display;
  PuglX11Clipboard* const board   = &view->impl->clipboard;

  if (typeIndex != board->acceptedFormatIndex) {
    return NULL;
  }

  const Window owner = XGetSelectionOwner(display, board->selection);
  if (!owner || owner != board->source) {
    *len = 0;
    return NULL;
  }

  *len = board->data.len;
  return board->data.data;
}

PuglStatus
puglAcceptOffer(PuglView* const                 view,
                const PuglDataOfferEvent* const offer,
                const uint32_t                  typeIndex)
{
  (void)offer;

  PuglInternals* const    impl    = view->impl;
  Display* const          display = view->world->impl->display;
  PuglX11Clipboard* const board   = &view->impl->clipboard;

  board->acceptedFormatIndex = typeIndex;
  board->acceptedFormat      = board->formats[typeIndex];

  // Request the data in the specified type from the general clipboard
  XConvertSelection(display,
                    board->selection,
                    board->acceptedFormat,
                    board->property,
                    impl->win,
                    CurrentTime);

  return PUGL_SUCCESS;
}

PuglStatus
puglPaste(PuglView* const view)
{
  Display* const          display = view->world->impl->display;
  const PuglX11Atoms*     atoms   = &view->world->impl->atoms;
  const PuglX11Clipboard* board   = &view->impl->clipboard;

  // Request a SelectionNotify for TARGETS (available datatypes)
  XConvertSelection(display,
                    board->selection,
                    atoms->TARGETS,
                    board->property,
                    view->impl->win,
                    CurrentTime);

  return PUGL_SUCCESS;
}

uint32_t
puglGetNumClipboardTypes(const PuglView* const view)
{
  return (uint32_t)view->impl->clipboard.numFormats;
}

const char*
puglGetClipboardType(const PuglView* const view, const uint32_t typeIndex)
{
  const PuglX11Clipboard* const board = &view->impl->clipboard;

  return typeIndex < board->numFormats ? board->formatStrings[typeIndex] : NULL;
}

PuglStatus
puglSetClipboard(PuglView* const   view,
                 const char* const type,
                 const void* const data,
                 const size_t      len)
{
  PuglInternals* const    impl    = view->impl;
  Display* const          display = view->world->impl->display;
  PuglX11Clipboard* const board   = &view->impl->clipboard;
  const PuglStatus        st      = puglSetBlob(&board->data, data, len);

  if (!st) {
    const Atom format = {XInternAtom(display, type, 0)};

    setClipboardFormats(view, board, 1, &format);
    XSetSelectionOwner(display, board->selection, impl->win, CurrentTime);

    board->source = impl->win;
  }

  return st;
}

PuglStatus
puglSetCursor(PuglView* const view, const PuglCursor cursor)
{
#ifdef HAVE_XCURSOR
  PuglInternals* const impl  = view->impl;
  const unsigned       index = (unsigned)cursor;
  const unsigned       count = sizeof(cursor_names) / sizeof(cursor_names[0]);
  if (index >= count) {
    return PUGL_BAD_PARAMETER;
  }

  const char* const name = cursor_names[index];
  if (!impl->win || impl->cursorName == name) {
    return PUGL_SUCCESS;
  }

  impl->cursorName = cursor_names[index];

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
  PuglInternals* const impl    = view->impl;
  Display* const       display = view->world->impl->display;
  XVisualInfo          pat     = PUGL_INIT_STRUCT;
  int                  n       = 0;

  pat.screen = impl->screen;
  if (!(impl->vi = XGetVisualInfo(display, VisualScreenMask, &pat, &n))) {
    return PUGL_BAD_CONFIGURATION;
  }

  view->hints[PUGL_RED_BITS]   = impl->vi->bits_per_rgb;
  view->hints[PUGL_GREEN_BITS] = impl->vi->bits_per_rgb;
  view->hints[PUGL_BLUE_BITS]  = impl->vi->bits_per_rgb;
  view->hints[PUGL_ALPHA_BITS] = 0;

  return PUGL_SUCCESS;
}
