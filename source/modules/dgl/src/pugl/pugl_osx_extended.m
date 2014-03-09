/*
  Copyright 2012 David Robillard <http://drobilla.net>
  Copyright 2013 Filipe Coelho <falktx@falktx.com>

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
   @file pugl_osx_extended.m Extended OSX/Cocoa Pugl Implementation.
*/

#import "pugl_osx.m"

#include "pugl_osx_extended.h"

void puglImplIdle(PuglView* view)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    NSEvent* event;

    static const NSUInteger eventMask = (NSLeftMouseDownMask | NSLeftMouseUpMask |
                                         NSRightMouseDownMask | NSRightMouseUpMask |
                                         NSMouseMovedMask |
                                         NSLeftMouseDraggedMask | NSRightMouseDraggedMask |
                                         NSMouseEnteredMask | NSMouseExitedMask |
                                         NSKeyDownMask | NSKeyUpMask |
                                         NSFlagsChangedMask |
                                         NSCursorUpdateMask | NSScrollWheelMask);

    for (;;) {
        event = [view->impl->window
                 nextEventMatchingMask:eventMask
                             untilDate:[NSDate distantPast]
                                inMode:NSEventTrackingRunLoopMode
                               dequeue:YES];

        if (event == nil)
            break;

        [view->impl->window sendEvent: event];
    }

    [pool release];
}

void puglImplFocus(PuglView* view)
{
    id window = view->impl->window;

    // TODO
    [NSApp activateIgnoringOtherApps:YES];
    [window makeKeyAndOrderFront:window];
}

void puglImplSetSize(PuglView* view, unsigned int width, unsigned int height, bool forced)
{
    id window = view->impl->window;

    NSRect frame      = [window frame];
    frame.origin.y   -= height - frame.size.height;
    frame.size.width  = width;
    frame.size.height = height+20;

//    if (forced) {
//        [window setFrame:frame];
//    } else {
        [window setFrame:frame display:YES animate:NO];
//    }

    // unused
    return; (void)forced;
}

void puglImplSetTitle(PuglView* view, const char* title)
{
    id window = view->impl->window;

    NSString* titleString = [[NSString alloc]
                              initWithBytes:title
                                     length:strlen(title)
                                   encoding:NSUTF8StringEncoding];

    [window setTitle:titleString];
}

void puglImplSetVisible(PuglView* view, bool yesNo)
{
    id window = view->impl->window;

    if (yesNo)
        [window setIsVisible:YES];
    else
        [window setIsVisible:NO];
}
