// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "win.h"

#include "internal.h"
#include "macros.h"
#include "platform.h"

#include <pugl/pugl.h>

#include <dwmapi.h>
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

#define PRE_20H1_DWMWA_USE_IMMERSIVE_DARK_MODE 19
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#  define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#define PUGL_LOCAL_CLOSE_MSG (WM_USER + 50)
#define PUGL_LOCAL_MARK_MSG (WM_USER + 51)
#define PUGL_LOCAL_CLIENT_MSG (WM_USER + 52)
#define PUGL_USER_TIMER_MIN 9470

#ifdef __cplusplus
#  define PUGL_INIT_STRUCT \
    {                      \
    }
#else
#  define PUGL_INIT_STRUCT {0}
#endif

typedef BOOL(WINAPI* PFN_SetProcessDPIAware)(void);
typedef HRESULT(WINAPI* PFN_GetProcessDpiAwareness)(HANDLE, DWORD*);
typedef HRESULT(WINAPI* PFN_GetScaleFactorForMonitor)(HMONITOR, DWORD*);

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef UNICODE

typedef wchar_t ArgStringChar;

static ArgStringChar*
puglArgStringNew(const char* const utf8)
{
  const int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
  if (len > 0) {
    wchar_t* result = (wchar_t*)calloc((size_t)len, sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result, len);
    return result;
  }

  return NULL;
}

static void
puglArgStringFree(ArgStringChar* const utf8)
{
  free(utf8);
}

#else // !defined(UNICODE)

typedef const char ArgStringChar;

static ArgStringChar*
puglArgStringNew(const char* const utf8)
{
  return utf8;
}

static void
puglArgStringFree(ArgStringChar* const utf8)
{
  (void)utf8;
}

#endif

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

static PuglStatus
puglWinStatus(const BOOL success)
{
  return success ? PUGL_SUCCESS : PUGL_UNKNOWN_ERROR;
}

static bool
puglRegisterWindowClass(const char* name)
{
  ArgStringChar* const nameArg = puglArgStringNew(name);

  HMODULE module = NULL;
  if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         (LPCTSTR)puglRegisterWindowClass,
                         &module)) {
    module = GetModuleHandle(NULL);
  }

  WNDCLASSEX wc = PUGL_INIT_STRUCT;
  if (GetClassInfoEx(module, nameArg, &wc)) {
    return true; // Already registered
  }

  wc.cbSize        = sizeof(wc);
  wc.style         = CS_OWNDC;
  wc.lpfnWndProc   = wndProc;
  wc.hInstance     = module;
  wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszClassName = nameArg;

  const bool success = !!RegisterClassEx(&wc);
  puglArgStringFree(nameArg);
  return success;
}

static unsigned
puglWinGetWindowFlags(const PuglView* const view)
{
  const unsigned commonFlags = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
  if (view->parent) {
    return commonFlags | WS_CHILD;
  }

  if (view->impl->fullscreen) {
    return commonFlags | WS_POPUPWINDOW;
  }

  const unsigned typeFlags =
    (view->hints[PUGL_VIEW_TYPE] == PUGL_VIEW_TYPE_DIALOG)
      ? (WS_DLGFRAME | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU)
      : (WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX);

  const bool     resizable = !!view->hints[PUGL_RESIZABLE];
  const unsigned sizeFlags = resizable ? (WS_SIZEBOX | WS_MAXIMIZEBOX) : 0U;

  return commonFlags | typeFlags | sizeFlags;
}

static unsigned
puglWinGetWindowExFlags(const PuglView* const view)
{
  return WS_EX_NOINHERITLAYOUT | (view->parent ? 0U : WS_EX_APPWINDOW);
}

static HWND
puglWinGetWindow(const PuglView* const view)
{
  return view->impl->hwnd        ? view->impl->hwnd
         : view->parent          ? (HWND)view->parent
         : view->transientParent ? (HWND)view->transientParent
                                 : NULL;
}

static HMONITOR
puglWinGetMonitor(const PuglView* const view)
{
  const HWND hwnd = puglWinGetWindow(view);
  if (hwnd) {
    return MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
  }

  const POINT point = {(long)view->lastConfigure.x,
                       (long)view->lastConfigure.y};
  return MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);
}

static double
puglWinGetViewScaleFactor(const PuglView* const view)
{
  const HMODULE shcore =
    LoadLibraryEx(TEXT("Shcore.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
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
    GetScaleFactorForMonitor(puglWinGetMonitor(view), &scaleFactor);
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
    HMODULE user32 =
      LoadLibraryEx(TEXT("user32.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
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

PuglStatus
puglRealize(PuglView* view)
{
  PuglInternals* impl = view->impl;
  PuglStatus     st   = PUGL_SUCCESS;

  // Ensure that we're unrealized
  if (impl->hwnd) {
    return PUGL_FAILURE;
  }

  // Check that the basic required configuration has been done
  if ((st = puglPreRealize(view))) {
    return st;
  }

  // Set default depth hints if the user hasn't specified any
  puglEnsureHint(view, PUGL_RED_BITS, 8);
  puglEnsureHint(view, PUGL_GREEN_BITS, 8);
  puglEnsureHint(view, PUGL_BLUE_BITS, 8);
  puglEnsureHint(view, PUGL_ALPHA_BITS, 8);

  // Get refresh rate for resize draw timer
  DEVMODE devMode;
  memset(&devMode, 0, sizeof(devMode));
  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
  view->hints[PUGL_REFRESH_RATE] = (int)devMode.dmDisplayFrequency;

  // Register window class if necessary
  if (!puglRegisterWindowClass(view->world->strings[PUGL_CLASS_NAME])) {
    return PUGL_REGISTRATION_FAILED;
  }

  // Configure and create window
  if ((st = view->backend->configure(view)) ||
      (st = view->backend->create(view))) {
    return st;
  }

  // Set basic window hints and attributes

  puglApplyViewString(
    view, PUGL_WINDOW_TITLE, view->strings[PUGL_WINDOW_TITLE]);
  puglSetTransientParent(view, view->transientParent);

  impl->scaleFactor = puglWinGetViewScaleFactor(view);
  impl->cursor      = LoadCursor(NULL, IDC_ARROW);

  if (view->hints[PUGL_DARK_FRAME]) {
    const BOOL useDarkMode = TRUE;
    if ((DwmSetWindowAttribute(impl->hwnd,
                               DWMWA_USE_IMMERSIVE_DARK_MODE,
                               &useDarkMode,
                               sizeof(useDarkMode)) != S_OK)) {
      DwmSetWindowAttribute(impl->hwnd,
                            PRE_20H1_DWMWA_USE_IMMERSIVE_DARK_MODE,
                            &useDarkMode,
                            sizeof(useDarkMode));
    }
  }

  SetWindowLongPtr(impl->hwnd, GWLP_USERDATA, (LONG_PTR)view);

  return puglDispatchSimpleEvent(view, PUGL_REALIZE);
}

PuglStatus
puglUnrealize(PuglView* const view)
{
  PuglInternals* const impl = view->impl;
  if (!impl || !impl->hwnd) {
    return PUGL_FAILURE;
  }

  puglDispatchSimpleEvent(view, PUGL_UNREALIZE);

  if (view->backend) {
    view->backend->destroy(view);
  }

  memset(&view->lastConfigure, 0, sizeof(PuglConfigureEvent));
  ReleaseDC(impl->hwnd, impl->hdc);
  impl->hdc = NULL;

  const PuglStatus st = puglWinStatus(DestroyWindow(impl->hwnd));
  impl->hwnd          = NULL;
  return st;
}

PuglStatus
puglShow(PuglView* view, const PuglShowCommand command)
{
  PuglInternals* impl = view->impl;
  PuglStatus     st   = impl->hwnd ? PUGL_SUCCESS : puglRealize(view);

  if (!st) {
    switch (command) {
    case PUGL_SHOW_PASSIVE:
      ShowWindow(impl->hwnd, SW_SHOWNOACTIVATE);
      break;
    case PUGL_SHOW_RAISE:
      ShowWindow(impl->hwnd, SW_SHOWNORMAL);
      st = SetActiveWindow(impl->hwnd) ? PUGL_SUCCESS : PUGL_FAILURE;
      break;
    case PUGL_SHOW_FORCE_RAISE:
      ShowWindow(impl->hwnd, SW_SHOWNORMAL);
      st = SetForegroundWindow(impl->hwnd) ? PUGL_SUCCESS : PUGL_FAILURE;
      break;
    }
  }

  return st;
}

PuglStatus
puglHide(PuglView* view)
{
  if (view->world->state == PUGL_WORLD_EXPOSING) {
    return PUGL_BAD_CALL;
  }

  ShowWindow(view->impl->hwnd, SW_HIDE);
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
  const char* const    className    = world->strings[PUGL_CLASS_NAME];
  ArgStringChar* const classNameArg = puglArgStringNew(className);

  UnregisterClass(classNameArg, NULL);
  puglArgStringFree(classNameArg);
  free(world->impl);
}

static PuglKey
keyInRange(const WPARAM  winSym,
           const WPARAM  winMin,
           const WPARAM  winMax,
           const PuglKey puglMin)
{
  return (winSym >= winMin && winSym <= winMax)
           ? (PuglKey)((WPARAM)puglMin + (winSym - winMin))
           : PUGL_KEY_NONE;
}

static PuglKey
keySymToSpecial(const WPARAM sym, const bool ext)
{
  PuglKey key = PUGL_KEY_NONE;
  if ((key = keyInRange(sym, VK_F1, VK_F12, PUGL_KEY_F1)) ||
      (key = keyInRange(sym,
                        VK_PRIOR,
                        VK_DOWN,
                        ext ? PUGL_KEY_PAGE_UP : PUGL_KEY_PAD_PAGE_UP)) ||
      (key = keyInRange(sym, VK_NUMPAD0, VK_NUMPAD9, PUGL_KEY_PAD_0)) ||
      (key = keyInRange(sym, VK_MULTIPLY, VK_DIVIDE, PUGL_KEY_PAD_MULTIPLY)) ||
      (key = keyInRange(sym, VK_LSHIFT, VK_RMENU, PUGL_KEY_SHIFT_L))) {
    return key;
  }

  // clang-format off
  switch (sym) {
  case VK_BACK:     return PUGL_KEY_BACKSPACE;
  case VK_CLEAR:    return PUGL_KEY_PAD_CLEAR;
  case VK_LWIN:     return PUGL_KEY_SUPER_L;
  case VK_RWIN:     return PUGL_KEY_SUPER_R;
  case VK_CAPITAL:  return PUGL_KEY_CAPS_LOCK;
  case VK_SCROLL:   return PUGL_KEY_SCROLL_LOCK;
  case VK_NUMLOCK:  return PUGL_KEY_NUM_LOCK;
  case VK_SNAPSHOT: return PUGL_KEY_PRINT_SCREEN;
  case VK_PAUSE:    return PUGL_KEY_PAUSE;
  }
  // clang-format on

  if (ext) {
    // clang-format off
    switch (sym) {
    case VK_RETURN:  return PUGL_KEY_PAD_ENTER;
    case VK_INSERT:  return PUGL_KEY_INSERT;
    case VK_DELETE:  return PUGL_KEY_DELETE;
    case VK_SHIFT:   return PUGL_KEY_SHIFT_R;
    case VK_CONTROL: return PUGL_KEY_CTRL_R;
    case VK_MENU:    return PUGL_KEY_ALT_R;
    }
    // clang-format on
  } else {
    // clang-format off
    switch (sym) {
    case VK_INSERT:  return PUGL_KEY_PAD_INSERT;
    case VK_DELETE:  return PUGL_KEY_PAD_DELETE;
    case VK_SHIFT:   return PUGL_KEY_SHIFT_L;
    case VK_CONTROL: return PUGL_KEY_CTRL_L;
    case VK_MENU:    return PUGL_KEY_ALT_L;
    }
    // clang-format on
  }

  return PUGL_KEY_NONE;
}

static bool
is_toggled(int vkey)
{
  return (unsigned)GetKeyState(vkey) & 1U;
}

static uint32_t
getModifiers(void)
{
  return ((uint32_t)(((GetKeyState(VK_SHIFT) < 0) ? PUGL_MOD_SHIFT : 0) |
                     ((GetKeyState(VK_CONTROL) < 0) ? PUGL_MOD_CTRL : 0) |
                     ((GetKeyState(VK_MENU) < 0) ? PUGL_MOD_ALT : 0) |
                     ((GetKeyState(VK_LWIN) < 0) ? PUGL_MOD_SUPER : 0) |
                     ((GetKeyState(VK_RWIN) < 0) ? PUGL_MOD_SUPER : 0) |
                     (is_toggled(VK_NUMLOCK) ? PUGL_MOD_NUM_LOCK : 0) |
                     (is_toggled(VK_SCROLL) ? PUGL_MOD_SCROLL_LOCK : 0) |
                     (is_toggled(VK_CAPITAL) ? PUGL_MOD_CAPS_LOCK : 0)));
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
  if (c0 >= 0xD800U && c0 < 0xDC00U) {
    if (len < 2) {
      return 0xFFFD; // Surrogate, but length is only 1
    }

    if (c1 >= 0xDC00U && c1 <= 0xDFFFU) {
      return ((c0 & 0x03FFU) << 10U) + (c1 & 0x03FFU) + 0x10000U;
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
  const bool     dead  = kchar >> (sizeof(UINT) * 8U - 1U) & 1U;
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

  const PuglKey special = keySymToSpecial(vkey, ext);
  if (special) {
    event->key   = (uint32_t)special;
    event->state = puglFilterMods(event->state, special);
  } else if (!dead) {
    // Translate unshifted key
    BYTE    keyboardState[256] = PUGL_INIT_STRUCT;
    wchar_t buf[5]             = PUGL_INIT_STRUCT;

    event->key = puglDecodeUTF16(
      buf, ToUnicode(vkey, vcode, keyboardState, buf, 4, 1 << 2));
  }
}

static void
initCharEvent(PuglEvent* event, PuglView* view, WPARAM wParam, LPARAM lParam)
{
  const wchar_t utf16[2] = {(wchar_t)(wParam & 0xFFFFU),
                            (wchar_t)((wParam >> 16U) & 0xFFFFU)};

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

  event->configure.type   = PUGL_CONFIGURE;
  event->configure.x      = (PuglCoord)rect.left;
  event->configure.y      = (PuglCoord)rect.top;
  event->configure.width  = (PuglSpan)width;
  event->configure.height = (PuglSpan)height;

  event->configure.style =
    ((view->impl->mapped ? (unsigned)PUGL_VIEW_STYLE_MAPPED : 0U) |
     (view->resizing ? (unsigned)PUGL_VIEW_STYLE_RESIZING : 0U) |
     (view->impl->fullscreen ? (unsigned)PUGL_VIEW_STYLE_FULLSCREEN : 0U) |
     (view->impl->minimized ? (unsigned)PUGL_VIEW_STYLE_HIDDEN : 0U) |
     (view->impl->maximized
        ? (unsigned)(PUGL_VIEW_STYLE_TALL | PUGL_VIEW_STYLE_WIDE)
        : 0U));

  return rect;
}

static void
handleCrossing(PuglView* view, const PuglEventType type, POINT pos)
{
  POINT root_pos = pos;
  ClientToScreen(view->impl->hwnd, &root_pos);

  const PuglCrossingEvent ev = {
    type,
    0U,
    GetMessageTime() / 1e3,
    (double)pos.x,
    (double)pos.y,
    (double)root_pos.x,
    (double)root_pos.y,
    getModifiers(),
    PUGL_CROSSING_NORMAL,
  };

  PuglEvent crossingEvent = {{type, 0U}};
  crossingEvent.crossing  = ev;
  puglDispatchEvent(view, &crossingEvent);
}

static void
constrainAspect(const PuglView* const view,
                RECT* const           size,
                const WPARAM          wParam)
{
  const PuglArea minAspect = view->sizeHints[PUGL_MIN_ASPECT];
  const PuglArea maxAspect = view->sizeHints[PUGL_MAX_ASPECT];

  const float minA = (float)minAspect.width / (float)minAspect.height;
  const float maxA = (float)maxAspect.width / (float)maxAspect.height;
  const float w    = (float)(size->right - size->left);
  const float h    = (float)(size->bottom - size->top);
  const float a    = w / h;

  switch (wParam) {
  case WMSZ_TOP:
    size->top = (a < minA   ? (LONG)((float)size->bottom - (w * minA))
                 : a > maxA ? (LONG)((float)size->bottom - (w * maxA))
                            : size->top);
    break;
  case WMSZ_TOPRIGHT:
  case WMSZ_RIGHT:
  case WMSZ_BOTTOMRIGHT:
    size->right = (a < minA   ? (LONG)((float)size->left + (h * minA))
                   : a > maxA ? (LONG)((float)size->left + (h * maxA))
                              : size->right);
    break;
  case WMSZ_BOTTOM:
    size->bottom = (a < minA   ? (LONG)((float)size->top + (w * minA))
                    : a > maxA ? (LONG)((float)size->top + (w * maxA))
                               : size->bottom);
    break;
  case WMSZ_BOTTOMLEFT:
  case WMSZ_LEFT:
  case WMSZ_TOPLEFT:
    size->left = (a < minA   ? (LONG)((float)size->right - (h * minA))
                  : a > maxA ? (LONG)((float)size->right - (h * maxA))
                             : size->left);
    break;
  }
}

static LRESULT
handleMessage(PuglView* view, UINT message, WPARAM wParam, LPARAM lParam)
{
  PuglEvent       event     = {{PUGL_NOTHING, 0U}};
  RECT            rect      = {0, 0, 0, 0};
  POINT           pt        = {0, 0};
  MINMAXINFO*     mmi       = NULL;
  void*           dummy_ptr = NULL;
  WINDOWPLACEMENT placement = {sizeof(WINDOWPLACEMENT), 0, 0, pt, pt, rect};

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
      RedrawWindow(view->impl->hwnd,
                   NULL,
                   NULL,
                   RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_INTERNALPAINT);
    }

    view->impl->mapped = wParam;
    handleConfigure(view, &event);
    break;
  case WM_DISPLAYCHANGE:
    view->impl->scaleFactor = puglWinGetViewScaleFactor(view);
    break;
  case WM_WINDOWPOSCHANGED:
    view->impl->minimized = false;
    view->impl->maximized = false;
    if (GetWindowPlacement(view->impl->hwnd, &placement)) {
      if (placement.showCmd == SW_SHOWMINIMIZED) {
        view->impl->minimized = true;
      } else if (placement.showCmd == SW_SHOWMAXIMIZED) {
        view->impl->maximized = true;
      }
    }
    handleConfigure(view, &event);
    break;
  case WM_SIZING:
    if (puglIsValidArea(view->sizeHints[PUGL_MIN_ASPECT]) &&
        puglIsValidArea(view->sizeHints[PUGL_MAX_ASPECT])) {
      constrainAspect(view, (RECT*)lParam, wParam);
      return TRUE;
    }
    break;
  case WM_ENTERSIZEMOVE:
    view->resizing = true;
    puglDispatchSimpleEvent(view, PUGL_LOOP_ENTER);
    handleConfigure(view, &event);
    break;
  case WM_ENTERMENULOOP:
    puglDispatchSimpleEvent(view, PUGL_LOOP_ENTER);
    break;
  case WM_TIMER:
    if (wParam >= PUGL_USER_TIMER_MIN) {
      PuglEvent ev = {{PUGL_TIMER, 0U}};
      ev.timer.id  = wParam - PUGL_USER_TIMER_MIN;
      puglDispatchEvent(view, &ev);
    }
    break;
  case WM_EXITSIZEMOVE:
    view->resizing = false;
    puglDispatchSimpleEvent(view, PUGL_LOOP_LEAVE);
    handleConfigure(view, &event);
    break;
  case WM_EXITMENULOOP:
    puglDispatchSimpleEvent(view, PUGL_LOOP_LEAVE);
    break;
  case WM_GETMINMAXINFO:
    mmi                   = (MINMAXINFO*)lParam;
    mmi->ptMinTrackSize.x = view->sizeHints[PUGL_MIN_SIZE].width;
    mmi->ptMinTrackSize.y = view->sizeHints[PUGL_MIN_SIZE].height;
    if (puglIsValidArea(view->sizeHints[PUGL_MAX_SIZE])) {
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
      TRACKMOUSEEVENT tme = PUGL_INIT_STRUCT;

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
  return puglWinStatus(!!SetFocus(view->impl->hwnd));
}

bool
puglHasFocus(const PuglView* view)
{
  return GetFocus() == view->impl->hwnd;
}

static bool
styleIsMaximized(const PuglViewStyleFlags flags)
{
  return (flags & PUGL_VIEW_STYLE_TALL) && (flags & PUGL_VIEW_STYLE_WIDE);
}

PuglStatus
puglSetViewStyle(PuglView* const view, const PuglViewStyleFlags flags)
{
  PuglInternals* const     impl     = view->impl;
  const PuglViewStyleFlags oldFlags = puglGetViewStyle(view);

  for (uint32_t mask = 1U; mask <= PUGL_MAX_VIEW_STYLE_FLAG; mask <<= 1U) {
    const bool oldValue = oldFlags & mask;
    const bool newValue = flags & mask;
    if (oldValue == newValue) {
      continue;
    }

    switch (mask) {
    case PUGL_VIEW_STYLE_MODAL:
    case PUGL_VIEW_STYLE_TALL:
    case PUGL_VIEW_STYLE_WIDE:
      break;

    case PUGL_VIEW_STYLE_HIDDEN:
      ShowWindow(impl->hwnd, newValue ? SW_SHOWMINIMIZED : SW_RESTORE);
      break;

    case PUGL_VIEW_STYLE_FULLSCREEN:
      impl->fullscreen = newValue;
      SetWindowLong(impl->hwnd, GWL_STYLE, (LONG)puglWinGetWindowFlags(view));
      SetWindowLong(
        impl->hwnd, GWL_EXSTYLE, (LONG)puglWinGetWindowExFlags(view));
      if (newValue) {
        GetWindowPlacement(impl->hwnd, &impl->oldPlacement);
        ShowWindow(impl->hwnd, SW_SHOWMAXIMIZED);
      } else {
        impl->oldPlacement.showCmd = SW_RESTORE;
        SetWindowPlacement(impl->hwnd, &impl->oldPlacement);
      }
      break;

    case PUGL_VIEW_STYLE_ABOVE:
    case PUGL_VIEW_STYLE_BELOW:
      break;

    case PUGL_VIEW_STYLE_DEMANDING: {
      FLASHWINFO info = {
        sizeof(FLASHWINFO), impl->hwnd, FLASHW_ALL | FLASHW_TIMERNOFG, 1, 0};

      FlashWindowEx(&info);
      break;
    }

    case PUGL_VIEW_STYLE_RESIZING:
      break;
    }
  }

  // Handle maximization (Windows doesn't have tall/wide styles)
  const bool oldMaximized = styleIsMaximized(oldFlags);
  const bool newMaximized = styleIsMaximized(flags);
  if (oldMaximized != newMaximized) {
    ShowWindow(impl->hwnd, newMaximized ? SW_SHOWMAXIMIZED : SW_RESTORE);
    puglObscureView(view);
  }

  return PUGL_SUCCESS;
}

PUGL_CONST_FUNC PuglStatus
puglApplySizeHint(PuglView* const    PUGL_UNUSED(view),
                  const PuglSizeHint PUGL_UNUSED(hint))
{
  return PUGL_SUCCESS;
}

PuglStatus
puglStartTimer(PuglView* view, uintptr_t id, double timeout)
{
  const UINT msec = (UINT)floor(timeout * 1000.0);

  SetTimer(view->impl->hwnd, PUGL_USER_TIMER_MIN + id, msec, NULL);
  return PUGL_SUCCESS;
}

PuglStatus
puglStopTimer(PuglView* view, uintptr_t id)
{
  return puglWinStatus(KillTimer(view->impl->hwnd, PUGL_USER_TIMER_MIN + id));
}

PuglStatus
puglSendEvent(PuglView* view, const PuglEvent* event)
{
  if (!view->impl->hwnd || view->world->state == PUGL_WORLD_EXPOSING) {
    return PUGL_FAILURE;
  }

  if (event->type == PUGL_CLOSE) {
    return puglWinStatus(PostMessage(view->impl->hwnd, WM_CLOSE, 0, 0));
  }

  if (event->type == PUGL_CLIENT) {
    return puglWinStatus(PostMessage(view->impl->hwnd,
                                     PUGL_LOCAL_CLIENT_MSG,
                                     (WPARAM)event->client.data1,
                                     (LPARAM)event->client.data2));
  }

  return PUGL_UNSUPPORTED;
}

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
  HWND hwnd = view->world->type == PUGL_PROGRAM ? NULL : view->impl->hwnd;
  while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
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
  static const double minWaitSeconds = 0.002;

  const double         startTime  = puglGetTime(world);
  const PuglWorldState startState = world->state;
  PuglStatus           st         = PUGL_SUCCESS;

  if (startState == PUGL_WORLD_IDLE) {
    world->state = PUGL_WORLD_UPDATING;
  } else if (startState != PUGL_WORLD_RECURSING) {
    return PUGL_BAD_CALL;
  }

  if (timeout < 0.0) {
    WaitMessage();
    st = puglDispatchWinEvents(world);
  } else if (timeout < minWaitSeconds) {
    st = puglDispatchWinEvents(world);
  } else {
    const double endTime = startTime + timeout - minWaitSeconds;
    double       t       = startTime;
    while (!st && t < endTime) {
      const DWORD timeoutMs = (DWORD)((endTime - t) * 1e3);
      MsgWaitForMultipleObjects(0, NULL, FALSE, timeoutMs, QS_ALLEVENTS);
      st = puglDispatchWinEvents(world);
      t  = puglGetTime(world);
    }
  }

  for (size_t i = 0; i < world->numViews; ++i) {
    if (puglGetVisible(world->views[i])) {
      puglDispatchSimpleEvent(world->views[i], PUGL_UPDATE);
    }

    UpdateWindow(world->views[i]->impl->hwnd);
  }

  world->state = startState;
  return st;
}

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
  return (((double)count.QuadPart / world->impl->timerFrequency) -
          world->startTime);
}

PuglStatus
puglObscureView(PuglView* view)
{
  return view->world->state == PUGL_WORLD_EXPOSING
           ? PUGL_BAD_CALL
           : puglWinStatus(InvalidateRect(view->impl->hwnd, NULL, false));
}

PuglStatus
puglObscureRegion(PuglView* const view,
                  const int       x,
                  const int       y,
                  const unsigned  width,
                  const unsigned  height)
{
  if (view->world->state == PUGL_WORLD_EXPOSING) {
    return PUGL_BAD_CALL;
  }

  if (!puglIsValidPosition(x, y) || !puglIsValidSize(width, height)) {
    return PUGL_BAD_PARAMETER;
  }

  const int      cx = MAX(0, x);
  const int      cy = MAX(0, y);
  const unsigned cw = MIN(view->lastConfigure.width, width);
  const unsigned ch = MIN(view->lastConfigure.height, height);

  const RECT r = {cx, cy, cx + (long)cw, cy + (long)ch};
  return puglWinStatus(InvalidateRect(view->impl->hwnd, &r, false));
}

PuglNativeView
puglGetNativeView(const PuglView* view)
{
  return (PuglNativeView)view->impl->hwnd;
}

PuglStatus
puglApplyViewString(PuglView* const      view,
                    const PuglStringHint key,
                    const char* const    value)
{
  PuglStatus st = PUGL_SUCCESS;
  if (!view->impl->hwnd) {
    return st;
  }

  if (key == PUGL_WINDOW_TITLE) {
    ArgStringChar* const titleArg = puglArgStringNew(value);
    st = puglWinStatus(SetWindowText(view->impl->hwnd, titleArg));
    puglArgStringFree(titleArg);
  }

  return st;
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

  RECT rect = {x, y, x + width, y + height};
  AdjustWindowRectEx(&rect, flags, FALSE, exFlags);
  return rect;
}

double
puglGetScaleFactor(const PuglView* const view)
{
  return view->impl->scaleFactor > 0.0 ? view->impl->scaleFactor
                                       : puglWinGetViewScaleFactor(view);
}

PuglStatus
puglSetWindowPosition(PuglView* const view, const int x, const int y)
{
  const RECT rect = adjustedWindowRect(
    view, x, y, view->lastConfigure.width, view->lastConfigure.height);

  const UINT flags =
    SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOSIZE;

  return puglWinStatus(
    SetWindowPos(view->impl->hwnd, HWND_TOP, rect.left, rect.top, 0, 0, flags));
}

PuglStatus
puglSetWindowSize(PuglView* const view,
                  const unsigned  width,
                  const unsigned  height)
{
  const RECT rect = adjustedWindowRect(view,
                                       view->lastConfigure.x,
                                       view->lastConfigure.y,
                                       (long)width,
                                       (long)height);

  const UINT flags =
    SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE;

  return puglWinStatus(SetWindowPos(view->impl->hwnd,
                                    HWND_TOP,
                                    0,
                                    0,
                                    rect.right - rect.left,
                                    rect.bottom - rect.top,
                                    flags));
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
  return IsClipboardFormatAvailable(CF_UNICODETEXT) ? 1U : 0U;
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
    0U,
    GetMessageTime() / 1e3,
    0,
  };

  PuglEvent dataEvent;
  dataEvent.data = data;
  return puglDispatchEvent(view, &dataEvent);
}

const void*
puglGetClipboard(PuglView* const view,
                 const uint32_t  typeIndex,
                 size_t* const   len)
{
  PuglInternals* const impl = view->impl;

  if (typeIndex > 0U || !IsClipboardFormatAvailable(CF_UNICODETEXT)) {
    return NULL;
  }

  // Try to open the clipboard several times since others may have locked it
  BOOL                  opened    = FALSE;
  static const unsigned max_tries = 16U;
  for (unsigned i = 0U; !opened && i < max_tries; ++i) {
    opened = OpenClipboard(impl->hwnd);
    if (!opened) {
      Sleep(0);
    }
  }

  if (!opened) {
    return NULL;
  }

  HGLOBAL  mem  = GetClipboardData(CF_UNICODETEXT);
  wchar_t* wstr = mem ? (wchar_t*)GlobalLock(mem) : NULL;
  if (!wstr) {
    CloseClipboard();
    return NULL;
  }

  free(impl->clipboard.data);
  impl->clipboard.data = puglWideCharToUtf8(wstr, &impl->clipboard.len);

  GlobalUnlock(mem);
  CloseClipboard();

  *len = impl->clipboard.len;
  return impl->clipboard.data;
}

PuglStatus
puglSetClipboard(PuglView* const   view,
                 const char* const type,
                 const void* const data,
                 const size_t      len)
{
  PuglInternals* const impl = view->impl;

  PuglStatus st = puglSetBlob(&impl->clipboard, data, len);
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
    0U,
    GetMessageTime() / 1e3,
  };

  PuglEvent offerEvent;
  offerEvent.offer = offer;
  return puglDispatchEvent(view, &offerEvent);
}

static const TCHAR* const cursor_ids[] = {
  IDC_ARROW,    // ARROW
  IDC_IBEAM,    // CARET
  IDC_CROSS,    // CROSSHAIR
  IDC_HAND,     // HAND
  IDC_NO,       // NO
  IDC_SIZEWE,   // LEFT_RIGHT
  IDC_SIZENS,   // UP_DOWN
  IDC_SIZENWSE, // UP_LEFT_DOWN_RIGHT
  IDC_SIZENESW, // UP_RIGHT_DOWN_LEFT
  IDC_SIZEALL,  // ALL_SCROLL
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

  const DWORD dwFlags = hints[PUGL_DOUBLE_BUFFER] ? PFD_DOUBLEBUFFER : 0U;

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

PuglPoint
puglGetAncestorCenter(const PuglView* const view)
{
  RECT rect = {0, 0, 0, 0};
  GetWindowRect(view->transientParent ? (HWND)view->transientParent
                                      : GetDesktopWindow(),
                &rect);

  const PuglPoint center = {
    (PuglCoord)(rect.left + ((rect.right - rect.left) / 2)),
    (PuglCoord)(rect.top + ((rect.bottom - rect.top) / 2))};
  return center;
}

PuglStatus
puglWinCreateWindow(PuglView* const   view,
                    const char* const title,
                    HWND* const       hwnd,
                    HDC* const        hdc)
{
  const char* className = (const char*)view->world->strings[PUGL_CLASS_NAME];

  // The meaning of "parent" depends on the window type (WS_CHILD)
  PuglNativeView parent = view->parent ? view->parent : view->transientParent;

  // Calculate initial window rectangle
  const unsigned  winFlags   = puglWinGetWindowFlags(view);
  const unsigned  winExFlags = puglWinGetWindowExFlags(view);
  const PuglArea  size       = puglGetInitialSize(view);
  const PuglPoint pos        = puglGetInitialPosition(view, size);
  RECT            wr         = {(long)pos.x,
                                (long)pos.y,
                                (long)pos.x + size.width,
                                (long)pos.y + size.height};
  AdjustWindowRectEx(&wr, winFlags, FALSE, winExFlags);

  ArgStringChar* const classNameArg = puglArgStringNew(className);
  ArgStringChar* const titleArg     = puglArgStringNew(title);

  // Create window and get drawing context
  *hwnd = CreateWindowEx(winExFlags,
                         classNameArg,
                         titleArg,
                         winFlags,
                         wr.left,
                         wr.right,
                         wr.right - wr.left,
                         wr.bottom - wr.top,
                         (HWND)parent,
                         NULL,
                         NULL,
                         NULL);
  puglArgStringFree(titleArg);
  puglArgStringFree(classNameArg);
  if (!*hwnd) {
    return PUGL_REALIZE_FAILED;
  }

  SetWindowPos(view->impl->hwnd,
               HWND_TOP,
               wr.left,
               wr.top,
               wr.right - wr.left,
               wr.bottom - wr.top,
               SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

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
    st         = PUGL_SET_FORMAT_FAILED;
  }

  return st;
}

PuglStatus
puglWinEnter(PuglView* view, const PuglExposeEvent* expose)
{
  if (expose) {
    return puglWinStatus(!!BeginPaint(view->impl->hwnd, &view->impl->paint));
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglWinLeave(PuglView* view, const PuglExposeEvent* expose)
{
  if (expose) {
    EndPaint(view->impl->hwnd, &view->impl->paint);
  }

  return PUGL_SUCCESS;
}
