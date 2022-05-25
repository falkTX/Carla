/*
  Copyright 2012-2021 David Robillard <d@drobilla.net>

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

#ifndef PUGL_DETAIL_WIN_H
#define PUGL_DETAIL_WIN_H

#include "implementation.h"

#include "pugl/pugl.h"

#include <windows.h>

#include <stdbool.h>

typedef PIXELFORMATDESCRIPTOR PuglWinPFD;

struct PuglWorldInternalsImpl {
  double timerFrequency;
};

struct PuglInternalsImpl {
  PuglWinPFD   pfd;
  int          pfId;
  HWND         hwnd;
  HCURSOR      cursor;
  HDC          hdc;
  PuglSurface* surface;
  bool         flashing;
  bool         mouseTracked;
};

PUGL_API
PuglWinPFD
puglWinGetPixelFormatDescriptor(const PuglHints hints);

PUGL_API
PuglStatus
puglWinCreateWindow(PuglView* const   view,
                    const char* const title,
                    HWND* const       hwnd,
                    HDC* const        hdc);

PUGL_API
PuglStatus
puglWinConfigure(PuglView* view);

PUGL_API
PuglStatus
puglWinEnter(PuglView* view, const PuglExposeEvent* expose);

PUGL_API
PuglStatus
puglWinLeave(PuglView* view, const PuglExposeEvent* expose);

#endif // PUGL_DETAIL_WIN_H
