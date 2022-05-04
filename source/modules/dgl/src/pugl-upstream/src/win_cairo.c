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

#include "stub.h"
#include "types.h"
#include "win.h"

#include "pugl/cairo.h"

#include <cairo-win32.h>
#include <cairo.h>

#include <stdlib.h>

typedef struct {
  cairo_surface_t* surface;
  cairo_t*         cr;
  HDC              drawDc;
  HBITMAP          drawBitmap;
} PuglWinCairoSurface;

static PuglStatus
puglWinCairoCreateDrawContext(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  surface->drawDc     = CreateCompatibleDC(impl->hdc);
  surface->drawBitmap = CreateCompatibleBitmap(
    impl->hdc, (int)view->frame.width, (int)view->frame.height);

  DeleteObject(SelectObject(surface->drawDc, surface->drawBitmap));

  return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoDestroyDrawContext(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  DeleteDC(surface->drawDc);
  DeleteObject(surface->drawBitmap);

  surface->drawDc     = NULL;
  surface->drawBitmap = NULL;

  return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoConfigure(PuglView* view)
{
  const PuglStatus st = puglWinConfigure(view);

  if (!st) {
    view->impl->surface =
      (PuglWinCairoSurface*)calloc(1, sizeof(PuglWinCairoSurface));
  }

  return st;
}

static void
puglWinCairoClose(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  cairo_surface_destroy(surface->surface);

  surface->surface = NULL;
}

static PuglStatus
puglWinCairoOpen(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  if (!(surface->surface = cairo_win32_surface_create(surface->drawDc)) ||
      cairo_surface_status(surface->surface) ||
      !(surface->cr = cairo_create(surface->surface)) ||
      cairo_status(surface->cr)) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoDestroy(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  puglWinCairoClose(view);
  puglWinCairoDestroyDrawContext(view);
  free(surface);
  impl->surface = NULL;

  return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoEnter(PuglView* view, const PuglExposeEvent* expose)
{
  PuglStatus st = PUGL_SUCCESS;

  if (expose && !(st = puglWinCairoCreateDrawContext(view)) &&
      !(st = puglWinCairoOpen(view))) {
    PAINTSTRUCT ps;
    BeginPaint(view->impl->hwnd, &ps);
  }

  return st;
}

static PuglStatus
puglWinCairoLeave(PuglView* view, const PuglExposeEvent* expose)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  if (expose) {
    cairo_surface_flush(surface->surface);
    BitBlt(impl->hdc,
           0,
           0,
           (int)view->frame.width,
           (int)view->frame.height,
           surface->drawDc,
           0,
           0,
           SRCCOPY);

    puglWinCairoClose(view);
    puglWinCairoDestroyDrawContext(view);

    PAINTSTRUCT ps;
    EndPaint(view->impl->hwnd, &ps);
  }

  return PUGL_SUCCESS;
}

static void*
puglWinCairoGetContext(PuglView* view)
{
  return ((PuglWinCairoSurface*)view->impl->surface)->cr;
}

const PuglBackend*
puglCairoBackend()
{
  static const PuglBackend backend = {puglWinCairoConfigure,
                                      puglStubCreate,
                                      puglWinCairoDestroy,
                                      puglWinCairoEnter,
                                      puglWinCairoLeave,
                                      puglWinCairoGetContext};

  return &backend;
}
