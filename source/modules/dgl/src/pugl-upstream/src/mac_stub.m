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

#include "pugl/stub.h"

#import <Cocoa/Cocoa.h>

@interface PuglStubView : NSView
@end

@implementation PuglStubView {
@public
  PuglView* puglview;
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
puglMacStubCreate(PuglView* view)
{
  PuglInternals* impl     = view->impl;
  PuglStubView*  drawView = [PuglStubView alloc];

  drawView->puglview = view;
  [drawView
    initWithFrame:NSMakeRect(0, 0, view->frame.width, view->frame.height)];
  if (view->hints[PUGL_RESIZABLE]) {
    [drawView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  } else {
    [drawView setAutoresizingMask:NSViewNotSizable];
  }

  impl->drawView = drawView;
  return PUGL_SUCCESS;
}

static PuglStatus
puglMacStubDestroy(PuglView* view)
{
  PuglStubView* const drawView = (PuglStubView*)view->impl->drawView;

  [drawView removeFromSuperview];
  [drawView release];

  view->impl->drawView = nil;
  return PUGL_SUCCESS;
}

const PuglBackend*
puglStubBackend(void)
{
  static const PuglBackend backend = {puglStubConfigure,
                                      puglMacStubCreate,
                                      puglMacStubDestroy,
                                      puglStubEnter,
                                      puglStubLeave,
                                      puglStubGetContext};

  return &backend;
}
