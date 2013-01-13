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
   @file pugl_win.cpp Windows/WGL Pugl Implementation.
*/

#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>

#include "pugl_internal.h"

#ifndef WM_MOUSEWHEEL
#    define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WM_MOUSEHWHEEL
#    define WM_MOUSEHWHEEL 0x020E
#endif

struct PuglInternalsImpl {
	HWND  hwnd;
	HDC   hdc;
	HGLRC hglrc;
};

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

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

	WNDCLASS wc;
	wc.style         = CS_OWNDC;
	wc.lpfnWndProc   = wndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = 0;
	wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "Pugl";
	RegisterClass(&wc);

	impl->hwnd = CreateWindow(
		"Pugl", title,
		(addToDesktop ? WS_VISIBLE : 0) | (parent ? WS_CHILD : (WS_POPUPWINDOW | WS_CAPTION)),
		0, 0, width, height,
		(HWND)parent, NULL, NULL, NULL);
	if (!impl->hwnd) {
		free(impl);
		free(view);
		return NULL;
	}
		
	SetWindowLongPtr(impl->hwnd, GWL_USERDATA, (LONG)view);

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
	wglMakeCurrent(impl->hdc, impl->hglrc);

	view->width  = width;
	view->height = height;

	return view;

	// unused
	(void)resizable;
	(void)addToDesktop;
	(void)x11Display;
}

void
puglDestroy(PuglView* view)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(view->impl->hglrc);
	ReleaseDC(view->impl->hwnd, view->impl->hdc);
	DestroyWindow(view->impl->hwnd);
	free(view->impl);
	free(view);
}

void
puglReshape(PuglView* view, int width, int height)
{
	wglMakeCurrent(view->impl->hdc, view->impl->hglrc);

	if (view->reshapeFunc) {
		view->reshapeFunc(view, width, height);
	} else {
		puglDefaultReshape(view, width, height);
	}

	view->width  = width;
	view->height = height;
}

void
puglDisplay(PuglView* view)
{
	wglMakeCurrent(view->impl->hdc, view->impl->hglrc);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	if (view->displayFunc) {
		view->displayFunc(view);
	}

	glFlush();
	SwapBuffers(view->impl->hdc);
	view->redisplay = false;
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

	setModifiers(view);
	switch (message) {
	case WM_CREATE:
	case WM_SHOWWINDOW:
	case WM_SIZE:
		puglReshape(view, view->width, view->height);
		break;
	case WM_PAINT:
		BeginPaint(view->impl->hwnd, &ps);
		puglDisplay(view);
		EndPaint(view->impl->hwnd, &ps);
		break;
	case WM_MOUSEMOVE:
		if (view->motionFunc) {
			view->motionFunc(
				view, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
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
			view->scrollFunc(
				view, 0, (int16_t)HIWORD(wParam) / (float)WHEEL_DELTA);
		}
		break;
	case WM_MOUSEHWHEEL:
		if (view->scrollFunc) {
			view->scrollFunc(
				view, (int16_t)HIWORD(wParam) / float(WHEEL_DELTA), 0);
		}
		break;
	case WM_KEYDOWN:
		if (view->ignoreKeyRepeat && (lParam & (1 << 30))) {
			break;
		} // else nobreak
	case WM_KEYUP:
		if ((key = keySymToSpecial(wParam))) {
			if (view->specialFunc) {
				view->specialFunc(view, message == WM_KEYDOWN, key);
			}
		} else if (view->keyboardFunc) {
			view->keyboardFunc(view, message == WM_KEYDOWN, wParam);
		}
		break;
	case WM_QUIT:
		if (view->closeFunc) {
			view->closeFunc(view);
		}
		break;
	default:
		return DefWindowProc(
			view->impl->hwnd, message, wParam, lParam);
	}

	return 0;
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	MSG msg;
	while (PeekMessage(&msg, view->impl->hwnd, 0, 0, PM_REMOVE)) {
		handleMessage(view, msg.message, msg.wParam, msg.lParam);
	}

	if (view->redisplay) {
		puglDisplay(view);
	}

	return PUGL_SUCCESS;
}

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PuglView* view = (PuglView*)GetWindowLongPtr(hwnd, GWL_USERDATA);
	switch (message) {
	case WM_CREATE:
		PostMessage(hwnd, WM_SHOWWINDOW, TRUE, 0);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_DESTROY:
		return 0;
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		PostMessage(hwnd, message, wParam, lParam);
		return 0;
	default:
		if (view) {
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
