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
   @file pugl_win.cpp Windows/WGL Pugl Implementation.
*/

#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>

#include <ctime>
#include <cstdio>
#include <cstdlib>

#include "pugl/pugl_internal.h"

#ifndef WM_MOUSEWHEEL
#    define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WM_MOUSEHWHEEL
#    define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef WHEEL_DELTA
#    define WHEEL_DELTA 120
#endif
#ifndef GWLP_USERDATA
#    define GWLP_USERDATA (-21)
#endif

#define PUGL_LOCAL_CLOSE_MSG (WM_USER + 50)

HINSTANCE hInstance = NULL;

struct PuglInternalsImpl {
	HWND     hwnd;
	HDC      hdc;
	HGLRC    hglrc;
	WNDCLASS wc;
};

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#if 0
extern "C" {
BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD, LPVOID)
{
    hInstance = hInst;
    return 1;
}
} // extern "C"
#endif

PuglInternals*
puglInitInternals()
{
	return (PuglInternals*)calloc(1, sizeof(PuglInternals));
}

void
puglEnterContext(PuglView* view)
{
#ifdef PUGL_HAVE_GL
	if (view->ctx_type == PUGL_GL) {
		wglMakeCurrent(view->impl->hdc, view->impl->hglrc);
	}
#endif
}

void
puglLeaveContext(PuglView* view, bool flush)
{
#ifdef PUGL_HAVE_GL
	if (view->ctx_type == PUGL_GL) {
		if (flush) {
			glFlush();
			SwapBuffers(view->impl->hdc);
		}
		wglMakeCurrent(NULL, NULL);
	}
#endif
}

int
puglCreateWindow(PuglView* view, const char* title)
{
	PuglInternals* impl = view->impl;

	if (!title) {
		title = "Window";
	}

	// FIXME: This is nasty, and pugl should not have static anything.
	// Should class be a parameter?  Does this make sense on other platforms?
	static int wc_count = 0;
	char classNameBuf[256];
	std::srand((std::time(NULL)));
	_snprintf(classNameBuf, sizeof(classNameBuf), "%s_%d-%d", title, std::rand(), ++wc_count);
	classNameBuf[sizeof(classNameBuf)-1] = '\0';

	impl->wc.style         = CS_OWNDC;
	impl->wc.lpfnWndProc   = wndProc;
	impl->wc.cbClsExtra    = 0;
	impl->wc.cbWndExtra    = 0;
	impl->wc.hInstance     = hInstance;
	impl->wc.hIcon         = LoadIcon(hInstance, IDI_APPLICATION);
	impl->wc.hCursor       = LoadCursor(hInstance, IDC_ARROW);
	impl->wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	impl->wc.lpszMenuName  = NULL;
	impl->wc.lpszClassName = strdup(classNameBuf);

	if (!RegisterClass(&impl->wc)) {
		free((void*)impl->wc.lpszClassName);
		free(impl);
		free(view);
		return 1;
	}

	int winFlags = WS_POPUPWINDOW | WS_CAPTION;
	if (view->resizable) {
		winFlags |= WS_SIZEBOX;
		if (view->min_width > 0 && view->min_height > 0) {
			// Adjust the minimum window size to accomodate requested view size
			RECT mr = { 0, 0, view->min_width, view->min_height };
			AdjustWindowRectEx(&mr, view->parent ? WS_CHILD : winFlags, FALSE, WS_EX_TOPMOST);
			view->min_width  = mr.right - mr.left;
			view->min_height = mr.bottom - mr.top;
		}
	}

	// Adjust the window size to accomodate requested view size
	RECT wr = { 0, 0, view->width, view->height };
	AdjustWindowRectEx(&wr, view->parent ? WS_CHILD : winFlags, FALSE, WS_EX_TOPMOST);

	impl->hwnd = CreateWindowEx(
		WS_EX_TOPMOST,
		classNameBuf, title,
		view->parent ? (WS_CHILD | WS_VISIBLE) : winFlags,
		CW_USEDEFAULT, CW_USEDEFAULT, wr.right-wr.left, wr.bottom-wr.top,
		(HWND)view->parent, NULL, hInstance, NULL);

	if (!impl->hwnd) {
		UnregisterClass(impl->wc.lpszClassName, NULL);
		free((void*)impl->wc.lpszClassName);
		free(impl);
		free(view);
		return 1;
	}

	SetWindowLongPtr(impl->hwnd, GWLP_USERDATA, (LONG_PTR)view);

	impl->hdc = GetDC(impl->hwnd);

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize      = sizeof(pfd);
	pfd.nVersion   = 1;
	pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int format = ChoosePixelFormat(impl->hdc, &pfd);
	SetPixelFormat(impl->hdc, format, &pfd);

	impl->hglrc = wglCreateContext(impl->hdc);
	if (!impl->hglrc) {
		ReleaseDC (impl->hwnd, impl->hdc);
		DestroyWindow (impl->hwnd);
		UnregisterClass (impl->wc.lpszClassName, NULL);
		free((void*)impl->wc.lpszClassName);
		free(impl);
		free(view);
		return 1;
	}

	return PUGL_SUCCESS;
}

void
puglShowWindow(PuglView* view)
{
	ShowWindow(view->impl->hwnd, SW_SHOWNORMAL);
}

void
puglHideWindow(PuglView* view)
{
	ShowWindow(view->impl->hwnd, SW_HIDE);
}

void
puglDestroy(PuglView* view)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(view->impl->hglrc);
	ReleaseDC(view->impl->hwnd, view->impl->hdc);
	DestroyWindow(view->impl->hwnd);
	UnregisterClass(view->impl->wc.lpszClassName, NULL);
	free((void*)view->impl->wc.lpszClassName);
	free(view->impl);
	free(view);
}

static void
puglReshape(PuglView* view, int width, int height)
{
	puglEnterContext(view);

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
	puglEnterContext(view);

	view->redisplay = false;
	if (view->displayFunc) {
		view->displayFunc(view);
	}

	puglLeaveContext(view, true);
}

static PuglKey
keySymToSpecial(int sym)
{
	switch (sym) {
	case VK_F1:      return PUGL_KEY_F1;
	case VK_F2:      return PUGL_KEY_F2;
	case VK_F3:      return PUGL_KEY_F3;
	case VK_F4:      return PUGL_KEY_F4;
	case VK_F5:      return PUGL_KEY_F5;
	case VK_F6:      return PUGL_KEY_F6;
	case VK_F7:      return PUGL_KEY_F7;
	case VK_F8:      return PUGL_KEY_F8;
	case VK_F9:      return PUGL_KEY_F9;
	case VK_F10:     return PUGL_KEY_F10;
	case VK_F11:     return PUGL_KEY_F11;
	case VK_F12:     return PUGL_KEY_F12;
	case VK_LEFT:    return PUGL_KEY_LEFT;
	case VK_UP:      return PUGL_KEY_UP;
	case VK_RIGHT:   return PUGL_KEY_RIGHT;
	case VK_DOWN:    return PUGL_KEY_DOWN;
	case VK_PRIOR:   return PUGL_KEY_PAGE_UP;
	case VK_NEXT:    return PUGL_KEY_PAGE_DOWN;
	case VK_HOME:    return PUGL_KEY_HOME;
	case VK_END:     return PUGL_KEY_END;
	case VK_INSERT:  return PUGL_KEY_INSERT;
	case VK_SHIFT:   return PUGL_KEY_SHIFT;
	case VK_CONTROL: return PUGL_KEY_CTRL;
	case VK_MENU:    return PUGL_KEY_ALT;
	case VK_LWIN:    return PUGL_KEY_SUPER;
	case VK_RWIN:    return PUGL_KEY_SUPER;
	}
	return (PuglKey)0;
}

static void
processMouseEvent(PuglView* view, int button, bool press, LPARAM lParam)
{
	view->event_timestamp_ms = GetMessageTime();
	if (press) {
		SetCapture(view->impl->hwnd);
	} else {
		ReleaseCapture();
	}

	if (view->mouseFunc) {
		view->mouseFunc(view, button, press,
		                GET_X_LPARAM(lParam),
		                GET_Y_LPARAM(lParam));
	}
}

static void
setModifiers(PuglView* view)
{
	view->mods = 0;
	view->mods |= (GetKeyState(VK_SHIFT)   < 0) ? PUGL_MOD_SHIFT  : 0;
	view->mods |= (GetKeyState(VK_CONTROL) < 0) ? PUGL_MOD_CTRL   : 0;
	view->mods |= (GetKeyState(VK_MENU)    < 0) ? PUGL_MOD_ALT    : 0;
	view->mods |= (GetKeyState(VK_LWIN)    < 0) ? PUGL_MOD_SUPER  : 0;
	view->mods |= (GetKeyState(VK_RWIN)    < 0) ? PUGL_MOD_SUPER  : 0;
}

static LRESULT
handleMessage(PuglView* view, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	PuglKey     key;
	RECT        rect;
	MINMAXINFO* mmi;

	setModifiers(view);
	switch (message) {
	case WM_CREATE:
	case WM_SHOWWINDOW:
	case WM_SIZE:
		GetClientRect(view->impl->hwnd, &rect);
		puglReshape(view, rect.right, rect.bottom);
		view->width = rect.right;
		view->height = rect.bottom;
		break;
	case WM_GETMINMAXINFO:
		mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize.x = view->min_width;
		mmi->ptMinTrackSize.y = view->min_height;
		break;
	case WM_PAINT:
		BeginPaint(view->impl->hwnd, &ps);
		puglDisplay(view);
		EndPaint(view->impl->hwnd, &ps);
		break;
	case WM_MOUSEMOVE:
		if (view->motionFunc) {
			view->event_timestamp_ms = GetMessageTime();
			view->motionFunc(view, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_LBUTTONDOWN:
		processMouseEvent(view, 1, true, lParam);
		break;
	case WM_MBUTTONDOWN:
		processMouseEvent(view, 2, true, lParam);
		break;
	case WM_RBUTTONDOWN:
		processMouseEvent(view, 3, true, lParam);
		break;
	case WM_LBUTTONUP:
		processMouseEvent(view, 1, false, lParam);
		break;
	case WM_MBUTTONUP:
		processMouseEvent(view, 2, false, lParam);
		break;
	case WM_RBUTTONUP:
		processMouseEvent(view, 3, false, lParam);
		break;
	case WM_MOUSEWHEEL:
		if (view->scrollFunc) {
			view->event_timestamp_ms = GetMessageTime();
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ScreenToClient(view->impl->hwnd, &pt);
			view->scrollFunc(
				view, pt.x, pt.y,
				0.0f, GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
		}
		break;
	case WM_MOUSEHWHEEL:
		if (view->scrollFunc) {
			view->event_timestamp_ms = GetMessageTime();
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ScreenToClient(view->impl->hwnd, &pt);
			view->scrollFunc(
				view, pt.x, pt.y,
				GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, 0.0f);
		}
		break;
	case WM_KEYDOWN:
		if (view->ignoreKeyRepeat && (lParam & (1 << 30))) {
			break;
		} // else nobreak
	case WM_KEYUP:
		view->event_timestamp_ms = GetMessageTime();
		if ((key = keySymToSpecial(wParam))) {
			if (view->specialFunc) {
				view->specialFunc(view, message == WM_KEYDOWN, key);
			}
		} else if (view->keyboardFunc) {
			static BYTE kbs[256];
			if (GetKeyboardState(kbs) != FALSE) {
				char lb[2];
				UINT scanCode = (lParam >> 8) & 0xFFFFFF00;
				if ( 1 == ToAscii(wParam, scanCode, kbs, (LPWORD)lb, 0)) {
					view->keyboardFunc(view, message == WM_KEYDOWN, (char)lb[0]);
				}
			}
		}
		break;
	case WM_QUIT:
	case PUGL_LOCAL_CLOSE_MSG:
		if (view->closeFunc) {
			view->closeFunc(view);
			view->redisplay = false;
		}
		break;
	default:
		return DefWindowProc(
			view->impl->hwnd, message, wParam, lParam);
	}

	return 0;
}

void
puglGrabFocus(PuglView* view)
{
	// TODO
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	MSG msg;
	while (PeekMessage(&msg, view->impl->hwnd, 0, 0, PM_REMOVE)) {
		handleMessage(view, msg.message, msg.wParam, msg.lParam);
	}

	if (view->redisplay) {
		InvalidateRect(view->impl->hwnd, NULL, FALSE);
	}

	return PUGL_SUCCESS;
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

void
puglPostRedisplay(PuglView* view)
{
	view->redisplay = true;
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return (PuglNativeWindow)view->impl->hwnd;
}

void*
puglGetContext(PuglView* view)
{
#ifdef PUGL_HAVE_CAIRO
	if (view->ctx_type == PUGL_CAIRO) {
		// TODO
	}
#endif
	return NULL;
}
