/*
  Copyright 2019-2020 David Robillard <d@drobilla.net>

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

#include "implementation.h"
#include "mac.h"
#include "stub.h"

#include "pugl/cairo.h"

#include <cairo-quartz.h>

#import <Cocoa/Cocoa.h>

#include <assert.h>

@interface PuglCairoView : NSView
@end

@implementation PuglCairoView {
@public
  PuglView*        puglview;
  cairo_surface_t* surface;
  cairo_t*         cr;
}

- (id)initWithFrame:(NSRect)frame
{
  self = [super initWithFrame:frame];

  return self;
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize
{
  PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];

  [super resizeWithOldSuperviewSize:oldSize];
  [wrapper setReshaped];
}

- (void)drawRect:(NSRect)rect
{
  PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];
  [wrapper dispatchExpose:rect];
}

@end

static PuglStatus
puglMacCairoCreate(PuglView* view)
{
  PuglInternals* impl     = view->impl;
  PuglCairoView* drawView = [PuglCairoView alloc];

  drawView->puglview = view;
  [drawView initWithFrame:[impl->wrapperView bounds]];
  if (view->hints[PUGL_RESIZABLE]) {
    [drawView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  } else {
    [drawView setAutoresizingMask:NSViewNotSizable];
  }

  impl->drawView = drawView;
  return PUGL_SUCCESS;
}

static PuglStatus
puglMacCairoDestroy(PuglView* view)
{
  PuglCairoView* const drawView = (PuglCairoView*)view->impl->drawView;

  [drawView removeFromSuperview];
  [drawView release];

  view->impl->drawView = nil;
  return PUGL_SUCCESS;
}

static PuglStatus
puglMacCairoEnter(PuglView* view, const PuglExposeEvent* expose)
{
  PuglCairoView* const drawView = (PuglCairoView*)view->impl->drawView;
  if (!expose) {
    return PUGL_SUCCESS;
  }

  assert(!drawView->surface);
  assert(!drawView->cr);

  const double scale   = 1.0 / [[NSScreen mainScreen] backingScaleFactor];
  CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
  const CGSize sizePx  = {view->frame.width, view->frame.height};
  const CGSize sizePt  = CGContextConvertSizeToUserSpace(context, sizePx);

  // Convert coordinates to standard Cairo space
  CGContextTranslateCTM(context, 0.0, -sizePt.height);
  CGContextScaleCTM(context, scale, -scale);

  drawView->surface = cairo_quartz_surface_create_for_cg_context(
    context, (unsigned)sizePx.width, (unsigned)sizePx.height);

  drawView->cr = cairo_create(drawView->surface);

  return PUGL_SUCCESS;
}

static PuglStatus
puglMacCairoLeave(PuglView* view, const PuglExposeEvent* expose)
{
  PuglCairoView* const drawView = (PuglCairoView*)view->impl->drawView;
  if (!expose) {
    return PUGL_SUCCESS;
  }

  assert(drawView->surface);
  assert(drawView->cr);

  CGContextRef context = cairo_quartz_surface_get_cg_context(drawView->surface);

  cairo_destroy(drawView->cr);
  cairo_surface_destroy(drawView->surface);
  drawView->cr      = NULL;
  drawView->surface = NULL;

  CGContextFlush(context);

  return PUGL_SUCCESS;
}

static void*
puglMacCairoGetContext(PuglView* view)
{
  return ((PuglCairoView*)view->impl->drawView)->cr;
}

const PuglBackend*
puglCairoBackend(void)
{
  static const PuglBackend backend = {puglStubConfigure,
                                      puglMacCairoCreate,
                                      puglMacCairoDestroy,
                                      puglMacCairoEnter,
                                      puglMacCairoLeave,
                                      puglMacCairoGetContext};

  return &backend;
}
