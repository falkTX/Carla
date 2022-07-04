// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "win.h"

#include "internal.h"
#include "platform.h"

#include "pugl/pugl.h"

#include <windows.h>
#include <windowsx.h>

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#ifndef WM_MOUSEWHEEL
#  define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WM_MOUSEHWHEEL
#  define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef WHEEL_DELTA
#  define WHEEL_DELTA 120
#endif
#ifndef GWLP_USERDATA
#  define GWLP_USERDATA (-21)
#endif

#define PUGL_LOCAL_CLOSE_MSG (WM_USER + 50)
#define PUGL_LOCAL_MARK_MSG (WM_USER + 51)
#define PUGL_LOCAL_CLIENT_MSG (WM_USER + 52)
#define PUGL_USER_TIMER_MIN 9470

typedef BOOL(WINAPI* PFN_SetProcessDPIAware)(void);
typedef HRESULT(WINAPI* PFN_GetProcessDpiAwareness)(HANDLE, DWORD*);
typedef HRESULT(WINAPI* PFN_GetScaleFactorForMonitor)(HMONITOR, DWORD*);

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static wchar_t*
puglUtf8ToWideChar(const char* const utf8)
{
  const int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
  if (len > 0) {
    wchar_t* result = (wchar_t*)calloc((size_t)len, sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result, len);
    return result;
  }

  return NULL;
}

static char*
puglWideCharToUtf8(const wchar_t* const wstr, size_t* len)
{
  int n = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
  if (n > 0) {
    char* result = (char*)calloc((size_t)n, sizeof(char));
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result, n, NULL, NULL);
    *len = (size_t)n - 1;
    return result;
  }

  return NULL;
}

static bool
puglRegisterWindowClass(const char* name)
{
  HMODULE module = NULL;
  if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         (LPCTSTR)puglRegisterWindowClass,
                         &module)) {
    module = GetModuleHandle(NULL);
  }

  WNDCLASSEX wc = {0};
  if (GetClassInfoEx(module, name, &wc)) {
    return true; // Already registered
  }

  wc.cbSize        = sizeof(wc);
  wc.style         = CS_OWNDC;
  wc.lpfnWndProc   = wndProc;
  wc.hInstance     = module;
  wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszClassName = name;

  return !!RegisterClassEx(&wc);
}

static unsigned
puglWinGetWindowFlags(const PuglView* const view)
{
  const bool     resizable = !!view->hints[PUGL_RESIZABLE];
  const unsigned sizeFlags = resizable ? (WS_SIZEBOX | WS_MAXIMIZEBOX) : 0u;

  return (WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
          (view->parent
             ? WS_CHILD
             : (WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | sizeFlags)));
}

static unsigned
puglWinGetWindowExFlags(const PuglView* const view)
{
  return WS_EX_NOINHERITLAYOUT | (view->parent ? 0u : WS_EX_APPWINDOW);
}

static double
puglWinGetViewScaleFactor(HWND hwnd)
{
  const HMODULE shcore = LoadLibrary("Shcore.dll");
  if (!shcore) {
    return 1.0;
  }

  const PFN_GetProcessDpiAwareness GetProcessDpiAwareness =
    (PFN_GetProcessDpiAwareness)GetProcAddress(shcore,
                                               "GetProcessDpiAwareness");

  const PFN_GetScaleFactorForMonitor GetScaleFactorForMonitor =
    (PFN_GetScaleFactorForMonitor)GetProcAddress(shcore,
                                                 "GetScaleFactorForMonitor");

  DWORD dpiAware    = 0;
  DWORD scaleFactor = 100;
  if (GetProcessDpiAwareness && GetScaleFactorForMonitor &&
      !GetProcessDpiAwareness(NULL, &dpiAware) && dpiAware) {
    GetScaleFactorForMonitor(
      MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY),
      &scaleFactor);
  }

  FreeLibrary(shcore);
  return (double)scaleFactor / 100.0;
}

PuglWorldInternals*
puglInitWorldInternals(PuglWorldType type, PuglWorldFlags PUGL_UNUSED(flags))
{
  PuglWorldInternals* impl =
    (PuglWorldInternals*)calloc(1, sizeof(PuglWorldInternals));
  if (!impl) {
    return NULL;
  }

  if (type == PUGL_PROGRAM) {
    HMODULE user32 = LoadLibrary("user32.dll");
    if (user32) {
      PFN_SetProcessDPIAware SetProcessDPIAware =
        (PFN_SetProcessDPIAware)GetProcAddress(user32, "SetProcessDPIAware");
      if (SetProcessDPIAware) {
        SetProcessDPIAware();
      }

      FreeLibrary(user32);
    }
  }

  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  impl->timerFrequency = (double)frequency.QuadPart;

  return impl;
}

void*
puglGetNativeWorld(PuglWorld* PUGL_UNUSED(world))
{
  return GetModuleHandle(NULL);
}

PuglInternals*
puglInitViewInternals(PuglWorld* PUGL_UNUSED(world))
{
  return (PuglInternals*)calloc(1, sizeof(PuglInternals));
}

static PuglStatus
puglPollWinEvents(PuglWorld* world, const double timeout)
{
  (void)world;

  if (timeout < 0) {
    WaitMessage();
  } else {
    MsgWaitForMultipleObjects(
      0, NULL, FALSE, (DWORD)(timeout * 1e3), QS_ALLEVENTS);
  }
  return PUGL_SUCCESS;
}

PuglStatus
puglRealize(PuglView* view)
{
  PuglInternals* impl = view->impl;
  if (impl->hwnd) {
    return PUGL_FAILURE;
  }

  // Getting depth from the display mode seems tedious, just set usual values
  if (view->hints[PUGL_RED_BITS] == PUGL_DONT_CARE) {
    view->hints[PUGL_RED_BITS] = 8;
  }
  if (view->hints[PUGL_BLUE_BITS] == PUGL_DONT_CARE) {
    view->hints[PUGL_BLUE_BITS] = 8;
  }
  if (view->hints[PUGL_GREEN_BITS] == PUGL_DONT_CARE) {
    view->hints[PUGL_GREEN_BITS] = 8;
  }
  if (view->hints[PUGL_ALPHA_BITS] == PUGL_DONT_CARE) {
    view->hints[PUGL_ALPHA_BITS] = 8;
  }

  // Get refresh rate for resize draw timer
  DEVMODEA devMode;
  memset(&devMode, 0, sizeof(devMode));
  EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &devMode);
  view->hints[PUGL_REFRESH_RATE] = (int)devMode.dmDisplayFrequency;

  // Register window class if necessary
  if (!puglRegisterWindowClass(view->world->className)) {
    return PUGL_REGISTRATION_FAILED;
  }

  if (!view->backend || !view->backend->configure) {
    return PUGL_BAD_BACKEND;
  }

  PuglStatus st = PUGL_SUCCESS;
  if ((st = view->backend->configure(view)) ||
      (st = view->backend->create(view))) {
    return st;
  }

  if (view->title) {
    puglSetWindowTitle(view, view->title);
  }

  view->impl->scaleFactor = puglWinGetViewScaleFactor(view->impl->hwnd);
  view->impl->cursor      = LoadCursor(NULL, IDC_ARROW);

  puglSetFrame(view, view->frame);
  SetWindowLongPtr(impl->hwnd, GWLP_USERDATA, (LONG_PTR)view);

  puglDispatchSimpleEvent(view, PUGL_CREATE);

  return PUGL_SUCCESS;
}

PuglStatus
puglShow(PuglView* view)
{
  PuglInternals* impl = view->impl;

  if (!impl->hwnd) {
    const PuglStatus st = puglRealize(view);
    if (st) {
      return st;
    }
  }

  ShowWindow(impl->hwnd, SW_SHOWNORMAL);
  SetFocus(impl->hwnd);
  return PUGL_SUCCESS;
}

PuglStatus
puglHide(PuglView* view)
{
  PuglInternals* impl = view->impl;

  ShowWindow(impl->hwnd, SW_HIDE);
  return PUGL_SUCCESS;
}

void
puglFreeViewInternals(PuglView* view)
{
  if (view) {
    if (view->backend) {
      view->backend->destroy(view);
    }

    ReleaseDC(view->impl->hwnd, view->impl->hdc);
    DestroyWindow(view->impl->hwnd);
    free(view->impl);
  }
}

void
puglFreeWorldInternals(PuglWorld* world)
{
  UnregisterClass(world->className, NULL);
  free(world->impl);
}

static PuglKey
keySymToSpecial(WPARAM sym)
{
  // clang-format off
  switch (sym) {
  case VK_F1:       return PUGL_KEY_F1;
  case VK_F2:       return PUGL_KEY_F2;
  case VK_F3:       return PUGL_KEY_F3;
  case VK_F4:       return PUGL_KEY_F4;
  case VK_F5:       return PUGL_KEY_F5;
  case VK_F6:       return PUGL_KEY_F6;
  case VK_F7:       return PUGL_KEY_F7;
  case VK_F8:       return PUGL_KEY_F8;
  case VK_F9:       return PUGL_KEY_F9;
  case VK_F10:      return PUGL_KEY_F10;
  case VK_F11:      return PUGL_KEY_F11;
  case VK_F12:      return PUGL_KEY_F12;
  case VK_BACK:     return PUGL_KEY_BACKSPACE;
  case VK_DELETE:   return PUGL_KEY_DELETE;
  case VK_LEFT:     return PUGL_KEY_LEFT;
  case VK_UP:       return PUGL_KEY_UP;
  case VK_RIGHT:    return PUGL_KEY_RIGHT;
  case VK_DOWN:     return PUGL_KEY_DOWN;
  case VK_PRIOR:    return PUGL_KEY_PAGE_UP;
  case VK_NEXT:     return PUGL_KEY_PAGE_DOWN;
  case VK_HOME:     return PUGL_KEY_HOME;
  case VK_END:      return PUGL_KEY_END;
  case VK_INSERT:   return PUGL_KEY_INSERT;
  case VK_SHIFT:
  case VK_LSHIFT:   return PUGL_KEY_SHIFT_L;
  case VK_RSHIFT:   return PUGL_KEY_SHIFT_R;
  case VK_CONTROL:
  case VK_LCONTROL: return PUGL_KEY_CTRL_L;
  case VK_RCONTROL: return PUGL_KEY_CTRL_R;
  case VK_MENU:
  case VK_LMENU:    return PUGL_KEY_ALT_L;
  case VK_RMENU:    return PUGL_KEY_ALT_R;
  case VK_LWIN:     return PUGL_KEY_SUPER_L;
  case VK_RWIN:     return PUGL_KEY_SUPER_R;
  case VK_CAPITAL:  return PUGL_KEY_CAPS_LOCK;
  case VK_SCROLL:   return PUGL_KEY_SCROLL_LOCK;
  case VK_NUMLOCK:  return PUGL_KEY_NUM_LOCK;
  case VK_SNAPSHOT: return PUGL_KEY_PRINT_SCREEN;
  case VK_PAUSE:    return PUGL_KEY_PAUSE;
  }
  // clang-format on

  return (PuglKey)0;
}

static uint32_t
getModifiers(void)
{
  // clang-format off
  return (((GetKeyState(VK_SHIFT)   < 0) ? PUGL_MOD_SHIFT  : 0u) |
          ((GetKeyState(VK_CONTROL) < 0) ? PUGL_MOD_CTRL   : 0u) |
          ((GetKeyState(VK_MENU)    < 0) ? PUGL_MOD_ALT    : 0u) |
          ((GetKeyState(VK_LWIN)    < 0) ? PUGL_MOD_SUPER  : 0u) |
          ((GetKeyState(VK_RWIN)    < 0) ? PUGL_MOD_SUPER  : 0u));
  // clang-format on
}

static void
initMouseEvent(PuglEvent* event,
               PuglView*  view,
               int        button,
               bool       press,
               LPARAM     lParam)
{
  POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
  ClientToScreen(view->impl->hwnd, &pt);

  if (press) {
    SetCapture(view->impl->hwnd);
  } else {
    ReleaseCapture();
  }

  event->button.time   = GetMessageTime() / 1e3;
  event->button.type   = press ? PUGL_BUTTON_PRESS : PUGL_BUTTON_RELEASE;
  event->button.x      = GET_X_LPARAM(lParam);
  event->button.y      = GET_Y_LPARAM(lParam);
  event->button.xRoot  = pt.x;
  event->button.yRoot  = pt.y;
  event->button.state  = getModifiers();
  event->button.button = (uint32_t)button;
}

static void
initScrollEvent(PuglEvent* event, PuglView* view, LPARAM lParam)
{
  POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
  ScreenToClient(view->impl->hwnd, &pt);

  event->scroll.time  = GetMessageTime() / 1e3;
  event->scroll.type  = PUGL_SCROLL;
  event->scroll.x     = pt.x;
  event->scroll.y     = pt.y;
  event->scroll.xRoot = GET_X_LPARAM(lParam);
  event->scroll.yRoot = GET_Y_LPARAM(lParam);
  event->scroll.state = getModifiers();
  event->scroll.dx    = 0;
  event->scroll.dy    = 0;
}

/// Return the code point for buf, or the replacement character on error
static uint32_t
puglDecodeUTF16(const wchar_t* buf, const int len)
{
  const uint32_t c0 = buf[0];
  const uint32_t c1 = buf[0];
  if (c0 >= 0xD800u && c0 < 0xDC00u) {
    if (len < 2) {
      return 0xFFFD; // Surrogate, but length is only 1
    }

    if (c1 >= 0xDC00u && c1 <= 0xDFFFu) {
      return ((c0 & 0x03FFu) << 10u) + (c1 & 0x03FFu) + 0x10000u;
    }

    return 0xFFFD; // Unpaired surrogates
  }

  return c0;
}

static void
initKeyEvent(PuglKeyEvent* event,
             PuglView*     view,
             bool          press,
             WPARAM        wParam,
             LPARAM        lParam)
{
  POINT rpos = {0, 0};
  GetCursorPos(&rpos);

  POINT cpos = {rpos.x, rpos.y};
  ScreenToClient(view->impl->hwnd, &rpos);

  const unsigned scode = (uint32_t)((lParam & 0xFF0000) >> 16);
  const unsigned vkey =
    ((wParam == VK_SHIFT) ? MapVirtualKey(scode, MAPVK_VSC_TO_VK_EX)
                          : (unsigned)wParam);

  const unsigned vcode = MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
  const unsigned kchar = MapVirtualKey(vkey, MAPVK_VK_TO_CHAR);
  const bool     dead  = kchar >> (sizeof(UINT) * 8u - 1u) & 1u;
  const bool     ext   = lParam & 0x01000000;

  event->type    = press ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
  event->time    = GetMessageTime() / 1e3;
  event->state   = getModifiers();
  event->xRoot   = rpos.x;
  event->yRoot   = rpos.y;
  event->x       = cpos.x;
  event->y       = cpos.y;
  event->keycode = (uint32_t)((lParam & 0xFF0000) >> 16);
  event->key     = 0;

  const PuglKey special = keySymToSpecial(vkey);
  if (special) {
    if (ext && (special == PUGL_KEY_CTRL || special == PUGL_KEY_ALT)) {
      event->key = (uint32_t)special + 1u; // Right hand key
    } else {
      event->key = (uint32_t)special;
    }
  } else if (!dead) {
    // Translate unshifted key
    BYTE    keyboardState[256] = {0};
    wchar_t buf[5]             = {0};

    event->key = puglDecodeUTF16(
      buf, ToUnicode(vkey, vcode, keyboardState, buf, 4, 1 << 2));
  }
}

static void
initCharEvent(PuglEvent* event, PuglView* view, WPARAM wParam, LPARAM lParam)
{
  const wchar_t utf16[2] = {(wchar_t)(wParam & 0xFFFFu),
                            (wchar_t)((wParam >> 16u) & 0xFFFFu)};

  initKeyEvent(&event->key, view, true, wParam, lParam);
  event->type           = PUGL_TEXT;
  event->text.character = puglDecodeUTF16(utf16, 2);

  if (!WideCharToMultiByte(
        CP_UTF8, 0, utf16, 2, event->text.string, 8, NULL, NULL)) {
    memset(event->text.string, 0, 8);
  }
}

static bool
ignoreKeyEvent(PuglView* view, LPARAM lParam)
{
  return view->hints[PUGL_IGNORE_KEY_REPEAT] && (lParam & (1 << 30));
}

static RECT
handleConfigure(PuglView* view, PuglEvent* event)
{
  RECT rect;
  GetClientRect(view->impl->hwnd, &rect);
  MapWindowPoints(view->impl->hwnd,
                  view->parent ? (HWND)view->parent : HWND_DESKTOP,
                  (LPPOINT)&rect,
                  2);

  const LONG width  = rect.right - rect.left;
  const LONG height = rect.bottom - rect.top;

  view->frame.x = (PuglCoord)rect.left;
  view->frame.y = (PuglCoord)rect.top;

  event->configure.type   = PUGL_CONFIGURE;
  event->configure.x      = (PuglCoord)view->frame.x;
  event->configure.y      = (PuglCoord)view->frame.y;
  event->configure.width  = (PuglSpan)width;
  event->configure.height = (PuglSpan)height;

  return rect;
}

static void
handleCrossing(PuglView* view, const PuglEventType type, POINT pos)
{
  POINT root_pos = pos;
  ClientToScreen(view->impl->hwnd, &root_pos);

  const PuglCrossingEvent ev = {
    type,
    0,
    GetMessageTime() / 1e3,
    (double)pos.x,
    (double)pos.y,
    (double)root_pos.x,
    (double)root_pos.y,
    getModifiers(),
    PUGL_CROSSING_NORMAL,
  };

  PuglEvent crossingEvent = {{type, 0}};
  crossingEvent.crossing  = ev;
  puglDispatchEvent(view, &crossingEvent);
}

static void
constrainAspect(const PuglView* const view,
                RECT* const           size,
                const WPARAM          wParam)
{
  const PuglViewSize minAspect = view->sizeHints[PUGL_MIN_ASPECT];
  const PuglViewSize maxAspect = view->sizeHints[PUGL_MAX_ASPECT];

  const float minA = (float)minAspect.width / (float)minAspect.height;
  const float maxA = (float)maxAspect.width / (float)maxAspect.height;
  const float w    = (float)(size->right - size->left);
  const float h    = (float)(size->bottom - size->top);
  const float a    = w / h;

  switch (wParam) {
  case WMSZ_TOP:
    size->top = (a < minA   ? (LONG)((float)size->bottom - w * minA)
                 : a > maxA ? (LONG)((float)size->bottom - w * maxA)
                            : size->top);
    break;
  case WMSZ_TOPRIGHT:
  case WMSZ_RIGHT:
  case WMSZ_BOTTOMRIGHT:
    size->right = (a < minA   ? (LONG)((float)size->left + h * minA)
                   : a > maxA ? (LONG)((float)size->left + h * maxA)
                              : size->right);
    break;
  case WMSZ_BOTTOM:
    size->bottom = (a < minA   ? (LONG)((float)size->top + w * minA)
                    : a > maxA ? (LONG)((float)size->top + w * maxA)
                               : size->bottom);
    break;
  case WMSZ_BOTTOMLEFT:
  case WMSZ_LEFT:
  case WMSZ_TOPLEFT:
    size->left = (a < minA   ? (LONG)((float)size->right - h * minA)
                  : a > maxA ? (LONG)((float)size->right - h * maxA)
                             : size->left);
    break;
  }
}

static LRESULT
handleMessage(PuglView* view, UINT message, WPARAM wParam, LPARAM lParam)
{
  PuglEvent   event     = {{PUGL_NOTHING, 0}};
  RECT        rect      = {0, 0, 0, 0};
  POINT       pt        = {0, 0};
  MINMAXINFO* mmi       = NULL;
  void*       dummy_ptr = NULL;

  if (InSendMessageEx(dummy_ptr)) {
    event.any.flags |= PUGL_IS_SEND_EVENT;
  }

  switch (message) {
  case WM_SETCURSOR:
    if (LOWORD(lParam) == HTCLIENT) {
      SetCursor(view->impl->cursor);
    } else {
      return DefWindowProc(view->impl->hwnd, message, wParam, lParam);
    }
    break;
  case WM_SHOWWINDOW:
    if (wParam) {
      handleConfigure(view, &event);
      puglDispatchEvent(view, &event);
      event.type = PUGL_NOTHING;

      RedrawWindow(view->impl->hwnd,
                   NULL,
                   NULL,
                   RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_INTERNALPAINT);
    }

    event.any.type = wParam ? PUGL_MAP : PUGL_UNMAP;
    break;
  case WM_SIZE:
    if (wParam == SIZE_MINIMIZED) {
      event.type = PUGL_UNMAP;
    } else if (!view->visible) {
      event.type = PUGL_MAP;
    } else {
      handleConfigure(view, &event);
      InvalidateRect(view->impl->hwnd, NULL, false);
    }
    break;
  case WM_DISPLAYCHANGE:
    view->impl->scaleFactor = puglWinGetViewScaleFactor(view->impl->hwnd);
    break;
  case WM_WINDOWPOSCHANGED:
    handleConfigure(view, &event);
    break;
  case WM_SIZING:
    if (view->sizeHints[PUGL_MIN_ASPECT].width) {
      constrainAspect(view, (RECT*)lParam, wParam);
      return TRUE;
    }
    break;
  case WM_ENTERSIZEMOVE:
  case WM_ENTERMENULOOP:
    puglDispatchSimpleEvent(view, PUGL_LOOP_ENTER);
    break;
  case WM_TIMER:
    if (wParam >= PUGL_USER_TIMER_MIN) {
      PuglEvent ev = {{PUGL_TIMER, 0}};
      ev.timer.id  = wParam - PUGL_USER_TIMER_MIN;
      puglDispatchEvent(view, &ev);
    }
    break;
  case WM_EXITSIZEMOVE:
  case WM_EXITMENULOOP:
    puglDispatchSimpleEvent(view, PUGL_LOOP_LEAVE);
    break;
  case WM_GETMINMAXINFO:
    mmi                   = (MINMAXINFO*)lParam;
    mmi->ptMinTrackSize.x = view->sizeHints[PUGL_MIN_SIZE].width;
    mmi->ptMinTrackSize.y = view->sizeHints[PUGL_MIN_SIZE].height;
    if (view->sizeHints[PUGL_MAX_SIZE].width &&
        view->sizeHints[PUGL_MAX_SIZE].height) {
      mmi->ptMaxTrackSize.x = view->sizeHints[PUGL_MAX_SIZE].width;
      mmi->ptMaxTrackSize.y = view->sizeHints[PUGL_MAX_SIZE].height;
    }
    break;
  case WM_PAINT:
    GetUpdateRect(view->impl->hwnd, &rect, false);
    event.expose.type   = PUGL_EXPOSE;
    event.expose.x      = (PuglCoord)rect.left;
    event.expose.y      = (PuglCoord)rect.top;
    event.expose.width  = (PuglSpan)(rect.right - rect.left);
    event.expose.height = (PuglSpan)(rect.bottom - rect.top);
    break;
  case WM_ERASEBKGND:
    return true;
  case WM_MOUSEMOVE:
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    if (!view->impl->mouseTracked) {
      TRACKMOUSEEVENT tme = {0};

      tme.cbSize    = sizeof(tme);
      tme.dwFlags   = TME_LEAVE;
      tme.hwndTrack = view->impl->hwnd;
      TrackMouseEvent(&tme);

      handleCrossing(view, PUGL_POINTER_IN, pt);
      view->impl->mouseTracked = true;
    }

    ClientToScreen(view->impl->hwnd, &pt);
    event.motion.type  = PUGL_MOTION;
    event.motion.time  = GetMessageTime() / 1e3;
    event.motion.x     = GET_X_LPARAM(lParam);
    event.motion.y     = GET_Y_LPARAM(lParam);
    event.motion.xRoot = pt.x;
    event.motion.yRoot = pt.y;
    event.motion.state = getModifiers();
    break;
  case WM_MOUSELEAVE:
    GetCursorPos(&pt);
    ScreenToClient(view->impl->hwnd, &pt);
    handleCrossing(view, PUGL_POINTER_OUT, pt);
    view->impl->mouseTracked = false;
    break;
  case WM_LBUTTONDOWN:
    initMouseEvent(&event, view, 0, true, lParam);
    break;
  case WM_MBUTTONDOWN:
    initMouseEvent(&event, view, 2, true, lParam);
    break;
  case WM_RBUTTONDOWN:
    initMouseEvent(&event, view, 1, true, lParam);
    break;
  case WM_XBUTTONDOWN:
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
      initMouseEvent(&event, view, 3, true, lParam);
    } else {
      initMouseEvent(&event, view, 4, true, lParam);
    }
    break;
  case WM_LBUTTONUP:
    initMouseEvent(&event, view, 0, false, lParam);
    break;
  case WM_MBUTTONUP:
    initMouseEvent(&event, view, 2, false, lParam);
    break;
  case WM_RBUTTONUP:
    initMouseEvent(&event, view, 1, false, lParam);
    break;
  case WM_XBUTTONUP:
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
      initMouseEvent(&event, view, 3, false, lParam);
    } else {
      initMouseEvent(&event, view, 4, false, lParam);
    }
    break;
  case WM_MOUSEWHEEL:
    initScrollEvent(&event, view, lParam);
    event.scroll.dy = GET_WHEEL_DELTA_WPARAM(wParam) / (double)WHEEL_DELTA;
    event.scroll.direction =
      (event.scroll.dy > 0 ? PUGL_SCROLL_UP : PUGL_SCROLL_DOWN);
    break;
  case WM_MOUSEHWHEEL:
    initScrollEvent(&event, view, lParam);
    event.scroll.dx = GET_WHEEL_DELTA_WPARAM(wParam) / (double)WHEEL_DELTA;
    event.scroll.direction =
      (event.scroll.dx > 0 ? PUGL_SCROLL_RIGHT : PUGL_SCROLL_LEFT);
    break;
  case WM_KEYDOWN:
    if (!ignoreKeyEvent(view, lParam)) {
      initKeyEvent(&event.key, view, true, wParam, lParam);
    }
    break;
  case WM_KEYUP:
    initKeyEvent(&event.key, view, false, wParam, lParam);
    break;
  case WM_CHAR:
    initCharEvent(&event, view, wParam, lParam);
    break;
  case WM_SETFOCUS:
    event.type = PUGL_FOCUS_IN;
    break;
  case WM_KILLFOCUS:
    event.type = PUGL_FOCUS_OUT;
    break;
  case WM_SYSKEYDOWN:
    initKeyEvent(&event.key, view, true, wParam, lParam);
    break;
  case WM_SYSKEYUP:
    initKeyEvent(&event.key, view, false, wParam, lParam);
    break;
  case WM_SYSCHAR:
    return TRUE;
  case PUGL_LOCAL_CLIENT_MSG:
    event.client.type  = PUGL_CLIENT;
    event.client.data1 = (uintptr_t)wParam;
    event.client.data2 = (uintptr_t)lParam;
    break;
  case WM_QUIT:
  case PUGL_LOCAL_CLOSE_MSG:
    event.any.type = PUGL_CLOSE;
    break;
  default:
    return DefWindowProc(view->impl->hwnd, message, wParam, lParam);
  }

  puglDispatchEvent(view, &event);

  return 0;
}

PuglStatus
puglGrabFocus(PuglView* view)
{
  SetFocus(view->impl->hwnd);
  return PUGL_SUCCESS;
}

bool
puglHasFocus(const PuglView* view)
{
  return GetFocus() == view->impl->hwnd;
}

PuglStatus
puglRequestAttention(PuglView* view)
{
  FLASHWINFO info = {
    sizeof(FLASHWINFO), view->impl->hwnd, FLASHW_ALL | FLASHW_TIMERNOFG, 1, 0};

  FlashWindowEx(&info);

  return PUGL_SUCCESS;
}

PuglStatus
puglStartTimer(PuglView* view, uintptr_t id, double timeout)
{
  const UINT msec = (UINT)floor(timeout * 1000.0);

  return (SetTimer(view->impl->hwnd, PUGL_USER_TIMER_MIN + id, msec, NULL)
            ? PUGL_SUCCESS
            : PUGL_UNKNOWN_ERROR);
}

PuglStatus
puglStopTimer(PuglView* view, uintptr_t id)
{
  return (KillTimer(view->impl->hwnd, PUGL_USER_TIMER_MIN + id)
            ? PUGL_SUCCESS
            : PUGL_UNKNOWN_ERROR);
}

PuglStatus
puglSendEvent(PuglView* view, const PuglEvent* event)
{
  if (event->type == PUGL_CLIENT) {
    PostMessage(view->impl->hwnd,
                PUGL_LOCAL_CLIENT_MSG,
                (WPARAM)event->client.data1,
                (LPARAM)event->client.data2);

    return PUGL_SUCCESS;
  }

  return PUGL_UNSUPPORTED;
}

#ifndef PUGL_DISABLE_DEPRECATED
PuglStatus
puglWaitForEvent(PuglView* PUGL_UNUSED(view))
{
  WaitMessage();
  return PUGL_SUCCESS;
}
#endif

static PuglStatus
puglDispatchViewEvents(PuglView* view)
{
  /* Windows has no facility to process only currently queued messages, which
     causes the event loop to run forever in cases like mouse movement where
     the queue is constantly being filled with new messages.  To work around
     this, we post a message to ourselves before starting, record its time
     when it is received, then break the loop on the first message that was
     created afterwards. */

  long markTime = 0;
  MSG  msg;
  while (PeekMessage(&msg, view->impl->hwnd, 0, 0, PM_REMOVE)) {
    if (msg.message == PUGL_LOCAL_MARK_MSG) {
      markTime = GetMessageTime();
    } else {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      if (markTime != 0 && GetMessageTime() > markTime) {
        break;
      }
    }
  }

  return PUGL_SUCCESS;
}

static PuglStatus
puglDispatchWinEvents(PuglWorld* world)
{
  for (size_t i = 0; i < world->numViews; ++i) {
    PostMessage(world->views[i]->impl->hwnd, PUGL_LOCAL_MARK_MSG, 0, 0);
  }

  for (size_t i = 0; i < world->numViews; ++i) {
    puglDispatchViewEvents(world->views[i]);
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglUpdate(PuglWorld* world, double timeout)
{
  const double startTime = puglGetTime(world);
  PuglStatus   st        = PUGL_SUCCESS;

  if (timeout < 0.0) {
    st = puglPollWinEvents(world, timeout);
    st = st ? st : puglDispatchWinEvents(world);
  } else if (timeout == 0.0) {
    st = puglDispatchWinEvents(world);
  } else {
    const double endTime = startTime + timeout - 0.001;
    for (double t = startTime; t < endTime; t = puglGetTime(world)) {
      if ((st = puglPollWinEvents(world, endTime - t)) ||
          (st = puglDispatchWinEvents(world))) {
        break;
      }
    }
  }

  for (size_t i = 0; i < world->numViews; ++i) {
    if (world->views[i]->visible) {
      puglDispatchSimpleEvent(world->views[i], PUGL_UPDATE);
    }

    UpdateWindow(world->views[i]->impl->hwnd);
  }

  return st;
}

#ifndef PUGL_DISABLE_DEPRECATED
PuglStatus
puglProcessEvents(PuglView* view)
{
  return puglUpdate(view->world, 0.0);
}
#endif

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  PuglView* view = (PuglView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

  switch (message) {
  case WM_CREATE:
    PostMessage(hwnd, WM_SHOWWINDOW, TRUE, 0);
    return 0;
  case WM_CLOSE:
    PostMessage(hwnd, PUGL_LOCAL_CLOSE_MSG, wParam, lParam);
    return 0;
  case WM_DESTROY:
    return 0;
  default:
    if (view && hwnd == view->impl->hwnd) {
      return handleMessage(view, message, wParam, lParam);
    } else {
      return DefWindowProc(hwnd, message, wParam, lParam);
    }
  }
}

double
puglGetTime(const PuglWorld* world)
{
  LARGE_INTEGER count;
  QueryPerformanceCounter(&count);
  return ((double)count.QuadPart / world->impl->timerFrequency -
          world->startTime);
}

PuglStatus
puglPostRedisplay(PuglView* view)
{
  InvalidateRect(view->impl->hwnd, NULL, false);
  return PUGL_SUCCESS;
}

PuglStatus
puglPostRedisplayRect(PuglView* view, const PuglRect rect)
{
  const RECT r = {(long)floor(rect.x),
                  (long)floor(rect.y),
                  (long)ceil(rect.x + rect.width),
                  (long)ceil(rect.y + rect.height)};

  InvalidateRect(view->impl->hwnd, &r, false);

  return PUGL_SUCCESS;
}

PuglNativeView
puglGetNativeView(PuglView* view)
{
  return (PuglNativeView)view->impl->hwnd;
}

PuglStatus
puglSetWindowTitle(PuglView* view, const char* title)
{
  puglSetString(&view->title, title);

  if (view->impl->hwnd) {
    wchar_t* wtitle = puglUtf8ToWideChar(title);
    if (wtitle) {
      SetWindowTextW(view->impl->hwnd, wtitle);
      free(wtitle);
    }
  }

  return PUGL_SUCCESS;
}

static RECT
adjustedWindowRect(PuglView* const view,
                   const long      x,
                   const long      y,
                   const long      width,
                   const long      height)
{
  const unsigned flags   = puglWinGetWindowFlags(view);
  const unsigned exFlags = puglWinGetWindowExFlags(view);

  RECT rect = {(long)x, (long)y, (long)x + (long)width, (long)y + (long)height};
  AdjustWindowRectEx(&rect, flags, FALSE, exFlags);
  return rect;
}

double
puglGetScaleFactor(const PuglView* const view)
{
  if (view->impl->scaleFactor) {
    return view->impl->scaleFactor;
  }
  return puglWinGetViewScaleFactor(view->parent
    ? (HWND)view->parent
    : view->transientParent
    ? (HWND)view->transientParent
    : NULL);
}

PuglStatus
puglSetFrame(PuglView* view, const PuglRect frame)
{
  if (view->impl->hwnd) {
    const RECT rect =
      adjustedWindowRect(view, frame.x, frame.y, frame.width, frame.height);

    if (!SetWindowPos(view->impl->hwnd,
                      HWND_TOP,
                      rect.left,
                      rect.top,
                      rect.right - rect.left,
                      rect.bottom - rect.top,
                      SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER)) {
      return PUGL_UNKNOWN_ERROR;
    }
  }

  view->frame = frame;
  return PUGL_SUCCESS;
}

PuglStatus
puglSetPosition(PuglView* const view, const int x, const int y)
{
  if (x > INT16_MAX || y > INT16_MAX) {
    return PUGL_BAD_PARAMETER;
  }

  if (view->impl->hwnd) {
    const RECT rect =
      adjustedWindowRect(view, x, y, view->frame.width, view->frame.height);

    if (!SetWindowPos(view->impl->hwnd,
                      HWND_TOP,
                      rect.left,
                      rect.top,
                      0,
                      0,
                      SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER |
                        SWP_NOSIZE)) {
      return PUGL_UNKNOWN_ERROR;
    }
  }

  view->frame.x = (PuglCoord)x;
  view->frame.y = (PuglCoord)y;
  return PUGL_SUCCESS;
}

PuglStatus
puglSetSize(PuglView* const view, const unsigned width, const unsigned height)
{
  if (width > INT16_MAX || height > INT16_MAX) {
    return PUGL_BAD_PARAMETER;
  }

  if (view->impl->hwnd) {
    const RECT rect = adjustedWindowRect(
      view, view->frame.x, view->frame.y, (long)width, (long)height);

    if (!SetWindowPos(view->impl->hwnd,
                      HWND_TOP,
                      0,
                      0,
                      rect.right - rect.left,
                      rect.bottom - rect.top,
                      SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER |
                        SWP_NOMOVE)) {
      return PUGL_UNKNOWN_ERROR;
    }
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
  if ((unsigned)hint >= PUGL_NUM_SIZE_HINTS) {
    return PUGL_BAD_PARAMETER;
  }

  view->sizeHints[hint].width  = width;
  view->sizeHints[hint].height = height;
  return PUGL_SUCCESS;
}

PuglStatus
puglSetTransientParent(PuglView* view, PuglNativeView parent)
{
  if (view->parent) {
    return PUGL_FAILURE;
  }

  view->transientParent = parent;

  if (view->impl->hwnd) {
    SetWindowLongPtr(view->impl->hwnd, GWLP_HWNDPARENT, (LONG_PTR)parent);
    return GetLastError() == NO_ERROR ? PUGL_SUCCESS : PUGL_FAILURE;
  }

  return PUGL_SUCCESS;
}

uint32_t
puglGetNumClipboardTypes(const PuglView* const PUGL_UNUSED(view))
{
  return IsClipboardFormatAvailable(CF_UNICODETEXT) ? 1u : 0u;
}

const char*
puglGetClipboardType(const PuglView* const PUGL_UNUSED(view),
                     const uint32_t        typeIndex)
{
  return (typeIndex == 0 && IsClipboardFormatAvailable(CF_UNICODETEXT))
           ? "text/plain"
           : NULL;
}

PuglStatus
puglAcceptOffer(PuglView* const                 view,
                const PuglDataOfferEvent* const PUGL_UNUSED(offer),
                const uint32_t                  typeIndex)
{
  if (typeIndex != 0) {
    return PUGL_UNSUPPORTED;
  }

  const PuglDataEvent data = {
    PUGL_DATA,
    0,
    GetMessageTime() / 1e3,
    0,
  };

  PuglEvent dataEvent;
  dataEvent.data = data;
  puglDispatchEvent(view, &dataEvent);
  return PUGL_SUCCESS;
}

const void*
puglGetClipboard(PuglView* const view,
                 const uint32_t  typeIndex,
                 size_t* const   len)
{
  PuglInternals* const impl = view->impl;

  if (typeIndex > 0u || !IsClipboardFormatAvailable(CF_UNICODETEXT) ||
      !OpenClipboard(impl->hwnd)) {
    return NULL;
  }

  HGLOBAL  mem  = GetClipboardData(CF_UNICODETEXT);
  wchar_t* wstr = mem ? (wchar_t*)GlobalLock(mem) : NULL;
  if (!wstr) {
    CloseClipboard();
    return NULL;
  }

  free(view->impl->clipboard.data);
  view->impl->clipboard.data =
    puglWideCharToUtf8(wstr, &view->impl->clipboard.len);

  GlobalUnlock(mem);
  CloseClipboard();

  *len = view->impl->clipboard.len;
  return view->impl->clipboard.data;
}

PuglStatus
puglSetClipboard(PuglView* const   view,
                 const char* const type,
                 const void* const data,
                 const size_t      len)
{
  PuglInternals* const impl = view->impl;

  PuglStatus st = puglSetBlob(&view->impl->clipboard, data, len);
  if (st) {
    return st;
  }

  if (!!strcmp(type, "text/plain")) {
    return PUGL_UNSUPPORTED;
  }

  if (!OpenClipboard(impl->hwnd)) {
    return PUGL_UNKNOWN_ERROR;
  }

  // Measure string and allocate global memory for clipboard
  const char* str  = (const char*)data;
  const int   wlen = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
  HGLOBAL     mem =
    GlobalAlloc(GMEM_MOVEABLE, (size_t)(wlen + 1) * sizeof(wchar_t));
  if (!mem) {
    CloseClipboard();
    return PUGL_UNKNOWN_ERROR;
  }

  // Lock global memory
  wchar_t* wstr = (wchar_t*)GlobalLock(mem);
  if (!wstr) {
    GlobalFree(mem);
    CloseClipboard();
    return PUGL_UNKNOWN_ERROR;
  }

  // Convert string into global memory and set it as clipboard data
  MultiByteToWideChar(CP_UTF8, 0, str, (int)len, wstr, wlen);
  wstr[wlen] = 0;
  GlobalUnlock(mem);
  SetClipboardData(CF_UNICODETEXT, mem);
  CloseClipboard();
  return PUGL_SUCCESS;
}

PuglStatus
puglPaste(PuglView* const view)
{
  const PuglDataOfferEvent offer = {
    PUGL_DATA_OFFER,
    0,
    GetMessageTime() / 1e3,
  };

  PuglEvent offerEvent;
  offerEvent.offer = offer;
  puglDispatchEvent(view, &offerEvent);
  return PUGL_SUCCESS;
}

static const char* const cursor_ids[] = {
  IDC_ARROW,    // ARROW
  IDC_IBEAM,    // CARET
  IDC_CROSS,    // CROSSHAIR
  IDC_HAND,     // HAND
  IDC_NO,       // NO
  IDC_SIZEWE,   // LEFT_RIGHT
  IDC_SIZENS,   // UP_DOWN
  IDC_SIZENWSE, // DIAGONAL
  IDC_SIZENESW, // ANTI_DIAGONAL
};

PuglStatus
puglSetCursor(PuglView* view, PuglCursor cursor)
{
  PuglInternals* const impl  = view->impl;
  const unsigned       index = (unsigned)cursor;
  const unsigned       count = sizeof(cursor_ids) / sizeof(cursor_ids[0]);

  if (index >= count) {
    return PUGL_BAD_PARAMETER;
  }

  const HCURSOR cur = LoadCursor(NULL, cursor_ids[index]);
  if (!cur) {
    return PUGL_FAILURE;
  }

  impl->cursor = cur;
  if (impl->mouseTracked) {
    SetCursor(cur);
  }

  return PUGL_SUCCESS;
}

// Semi-public platform API used by backends

PuglWinPFD
puglWinGetPixelFormatDescriptor(const PuglHints hints)
{
  const int rgbBits = (hints[PUGL_RED_BITS] +   //
                       hints[PUGL_GREEN_BITS] + //
                       hints[PUGL_BLUE_BITS]);

  const DWORD dwFlags = hints[PUGL_DOUBLE_BUFFER] ? PFD_DOUBLEBUFFER : 0u;

  PuglWinPFD pfd;
  ZeroMemory(&pfd, sizeof(pfd));
  pfd.nSize        = sizeof(pfd);
  pfd.nVersion     = 1;
  pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | dwFlags;
  pfd.iPixelType   = PFD_TYPE_RGBA;
  pfd.cColorBits   = (BYTE)rgbBits;
  pfd.cRedBits     = (BYTE)hints[PUGL_RED_BITS];
  pfd.cGreenBits   = (BYTE)hints[PUGL_GREEN_BITS];
  pfd.cBlueBits    = (BYTE)hints[PUGL_BLUE_BITS];
  pfd.cAlphaBits   = (BYTE)hints[PUGL_ALPHA_BITS];
  pfd.cDepthBits   = (BYTE)hints[PUGL_DEPTH_BITS];
  pfd.cStencilBits = (BYTE)hints[PUGL_STENCIL_BITS];
  pfd.iLayerType   = PFD_MAIN_PLANE;
  return pfd;
}

PuglStatus
puglWinCreateWindow(PuglView* const   view,
                    const char* const title,
                    HWND* const       hwnd,
                    HDC* const        hdc)
{
  const char*    className  = (const char*)view->world->className;
  const unsigned winFlags   = puglWinGetWindowFlags(view);
  const unsigned winExFlags = puglWinGetWindowExFlags(view);

  if (view->frame.width <= 0.0 && view->frame.height <= 0.0) {
    const PuglViewSize defaultSize = view->sizeHints[PUGL_DEFAULT_SIZE];
    if (!defaultSize.width || !defaultSize.height) {
      return PUGL_BAD_CONFIGURATION;
    }

    RECT desktopRect;
    GetClientRect(GetDesktopWindow(), &desktopRect);

    const int screenWidth  = desktopRect.right - desktopRect.left;
    const int screenHeight = desktopRect.bottom - desktopRect.top;

    view->frame.width  = defaultSize.width;
    view->frame.height = defaultSize.height;

    if (!view->parent) {
      view->frame.x = (PuglCoord)((screenWidth - view->frame.width) / 2);
      view->frame.y = (PuglCoord)((screenHeight - view->frame.height) / 2);
    }
  }

  // The meaning of "parent" depends on the window type (WS_CHILD)
  PuglNativeView parent = view->parent ? view->parent : view->transientParent;

  // Calculate total window size to accommodate requested view size
  RECT wr = {(long)view->frame.x,
             (long)view->frame.y,
             (long)view->frame.width,
             (long)view->frame.height};
  AdjustWindowRectEx(&wr, winFlags, FALSE, winExFlags);

  // Create window and get drawing context
  if (!(*hwnd = CreateWindowEx(winExFlags,
                               className,
                               title,
                               winFlags,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               wr.right - wr.left,
                               wr.bottom - wr.top,
                               (HWND)parent,
                               NULL,
                               NULL,
                               NULL))) {
    return PUGL_REALIZE_FAILED;
  }

  if (!(*hdc = GetDC(*hwnd))) {
    DestroyWindow(*hwnd);
    *hwnd = NULL;
    return PUGL_REALIZE_FAILED;
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglWinConfigure(PuglView* view)
{
  PuglInternals* const impl = view->impl;
  PuglStatus           st   = PUGL_SUCCESS;

  if ((st = puglWinCreateWindow(view, "Pugl", &impl->hwnd, &impl->hdc))) {
    return st;
  }

  impl->pfd  = puglWinGetPixelFormatDescriptor(view->hints);
  impl->pfId = ChoosePixelFormat(impl->hdc, &impl->pfd);

  if (!SetPixelFormat(impl->hdc, impl->pfId, &impl->pfd)) {
    ReleaseDC(impl->hwnd, impl->hdc);
    DestroyWindow(impl->hwnd);
    impl->hwnd = NULL;
    impl->hdc  = NULL;
    return PUGL_SET_FORMAT_FAILED;
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglWinEnter(PuglView* view, const PuglExposeEvent* expose)
{
  if (expose) {
    PAINTSTRUCT ps;
    BeginPaint(view->impl->hwnd, &ps);
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglWinLeave(PuglView* view, const PuglExposeEvent* expose)
{
  if (expose) {
    PAINTSTRUCT ps;
    EndPaint(view->impl->hwnd, &ps);
  }

  return PUGL_SUCCESS;
}
