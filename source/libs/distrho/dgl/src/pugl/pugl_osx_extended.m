/*
  Copyright 2012 David Robillard <http://drobilla.net>
  Copyright 2013 Fil...

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

void puglImplFocus(PuglView* view)
{
	// TODO
}

void puglImplSetSize(PuglView* view, unsigned int width, unsigned int height)
{
	id window = view->impl->window;

	// TODO
//	NSRect frame = [window frame];
//	frame.size.width  = width;
//	frame.size.height = height;

	// display:NO ?
//	[window setFrame:frame display:YES animate:NO];
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

	if (yesNo) {
		[window setIsVisible:YES];
	} else {
		[window setIsVisible:NO];
	}
}
