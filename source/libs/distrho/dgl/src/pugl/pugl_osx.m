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
   @file pugl_osx.m OSX/Cocoa Pugl Implementation.
*/

#include <stdlib.h>

#import <Cocoa/Cocoa.h>

#include "pugl_internal.h"

@interface PuglOpenGLView : NSOpenGLView
{
	int colorBits;
	int depthBits;
@public
	PuglView* view;
}

- (id) initWithFrame:(NSRect)frame
           colorBits:(int)numColorBits
           depthBits:(int)numDepthBits;
- (void) reshape;
- (void) drawRect:(NSRect)rect;
- (void) mouseMoved:(NSEvent*)event;
- (void) mouseDown:(NSEvent*)event;
- (void) mouseUp:(NSEvent*)event;
- (void) rightMouseDown:(NSEvent*)event;
- (void) rightMouseUp:(NSEvent*)event;
- (void) keyDown:(NSEvent*)event;
- (void) keyUp:(NSEvent*)event;
- (void) flagsChanged:(NSEvent*)event;

@end

@implementation PuglOpenGLView

- (id) initWithFrame:(NSRect)frame
           colorBits:(int)numColorBits
           depthBits:(int)numDepthBits
{
	colorBits = numColorBits;
	depthBits = numDepthBits;

	NSOpenGLPixelFormatAttribute pixelAttribs[16] = {
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAColorSize,
		colorBits,
		NSOpenGLPFADepthSize,
		depthBits,
		0
	};

	NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc]
		              initWithAttributes:pixelAttribs];

	if (pixelFormat) {
		self = [super initWithFrame:frame pixelFormat:pixelFormat];
		[pixelFormat release];
		if (self) {
			[[self openGLContext] makeCurrentContext];
			[self reshape];
		}
	} else {
		self = nil;
	}

	return self;
}

- (void) reshape
{
	[[self openGLContext] update];

	NSRect bounds = [self bounds];
	int    width  = bounds.size.width;
	int    height = bounds.size.height;

	if (view->reshapeFunc) {
		view->reshapeFunc(view, width, height);
	} else {
		puglDefaultReshape(view, width, height);
	}

	view->width  = width;
	view->height = height;
}

- (void) drawRect:(NSRect)rect
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	if (self->view->displayFunc) {
		self->view->displayFunc(self->view);
	}

	glFlush();
	glSwapAPPLE();
}

static int
getModifiers(unsigned modifierFlags)
{
	int mods = 0;
	mods |= (modifierFlags & NSShiftKeyMask)     ? PUGL_MOD_SHIFT : 0;
	mods |= (modifierFlags & NSControlKeyMask)   ? PUGL_MOD_CTRL  : 0;
	mods |= (modifierFlags & NSAlternateKeyMask) ? PUGL_MOD_ALT   : 0;
	mods |= (modifierFlags & NSCommandKeyMask)   ? PUGL_MOD_SUPER : 0;
	return mods;
}

- (void) mouseMoved:(NSEvent*)event
{
	if (view->motionFunc) {
		NSPoint loc = [event locationInWindow];
		view->mods = getModifiers([event modifierFlags]);
		view->motionFunc(view, loc.x, loc.y);
	}
}

- (void) mouseDown:(NSEvent*)event
{
	if (view->mouseFunc) {
		NSPoint loc = [event locationInWindow];
		view->mods = getModifiers([event modifierFlags]);
		view->mouseFunc(view, 1, true, loc.x, loc.y);
	}
}

- (void) mouseUp:(NSEvent*)event
{
	if (view->mouseFunc) {
		NSPoint loc = [event locationInWindow];
		view->mods = getModifiers([event modifierFlags]);
		view->mouseFunc(view, 1, false, loc.x, loc.y);
	}
}

- (void) rightMouseDown:(NSEvent*)event
{
	if (view->mouseFunc) {
		NSPoint loc = [event locationInWindow];
		view->mods = getModifiers([event modifierFlags]);
		view->mouseFunc(view, 3, true, loc.x, loc.y);
	}
}

- (void) rightMouseUp:(NSEvent*)event
{
	if (view->mouseFunc) {
		NSPoint loc = [event locationInWindow];
		view->mods = getModifiers([event modifierFlags]);
		view->mouseFunc(view, 3, false, loc.x, loc.y);
	}
}

- (void) scrollWheel:(NSEvent*)event
{
	if (view->scrollFunc) {
		view->mods = getModifiers([event modifierFlags]);
		view->scrollFunc(view, [event deltaX], [event deltaY]);
	}
}

- (void) keyDown:(NSEvent*)event
{
	if (view->keyboardFunc && !(view->ignoreKeyRepeat && [event isARepeat])) {
		NSString* chars = [event characters];
		view->mods = getModifiers([event modifierFlags]);
		view->keyboardFunc(view, true, [chars characterAtIndex:0]);
	}
}

- (void) keyUp:(NSEvent*)event
{
	if (view->keyboardFunc) {
		NSString* chars = [event characters];
		view->mods = getModifiers([event modifierFlags]);
		view->keyboardFunc(view, false,  [chars characterAtIndex:0]);
	}
}

- (void) flagsChanged:(NSEvent*)event
{
	if (view->specialFunc) {
		int mods = getModifiers([event modifierFlags]);
		if ((mods & PUGL_MOD_SHIFT) != (view->mods & PUGL_MOD_SHIFT)) {
			view->specialFunc(view, mods & PUGL_MOD_SHIFT, PUGL_KEY_SHIFT);
		} else if ((mods & PUGL_MOD_CTRL) != (view->mods & PUGL_MOD_CTRL)) {
			view->specialFunc(view, mods & PUGL_MOD_CTRL, PUGL_KEY_CTRL);
		} else if ((mods & PUGL_MOD_ALT) != (view->mods & PUGL_MOD_ALT)) {
			view->specialFunc(view, mods & PUGL_MOD_ALT, PUGL_KEY_ALT);
		} else if ((mods & PUGL_MOD_SUPER) != (view->mods & PUGL_MOD_SUPER)) {
			view->specialFunc(view, mods & PUGL_MOD_SUPER, PUGL_KEY_SUPER);
		}
		view->mods = mods;
	}
}

@end

struct PuglInternalsImpl {
	PuglOpenGLView* view;
	NSModalSession  session;
	id              window;
};

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

	[NSAutoreleasePool new];
	[NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

	NSString* titleString = [[NSString alloc]
		                        initWithBytes:title
		                               length:strlen(title)
		                             encoding:NSUTF8StringEncoding];

	id window = [[[NSWindow alloc]
		             initWithContentRect:NSMakeRect(0, 0, 512, 512)
		                       styleMask:NSTitledWindowMask
		                         backing:NSBackingStoreBuffered
		                           defer:NO]
		            autorelease];

	[window cascadeTopLeftFromPoint:NSMakePoint(20, 20)];
	[window setTitle:titleString];
	[window setAcceptsMouseMovedEvents:YES];

	impl->view       = [PuglOpenGLView new];
	impl->window     = window;
	impl->view->view = view;

	[window setContentView:impl->view];
	[NSApp activateIgnoringOtherApps:YES];
	[window makeFirstResponder:impl->view];

	impl->session = [NSApp beginModalSessionForWindow:view->impl->window];

	return view;

	// unused
	(void)addToDesktop;
	(void)x11Display;
}

void
puglDestroy(PuglView* view)
{
	[NSApp endModalSession:view->impl->session];
	[view->impl->view release];
	free(view->impl);
	free(view);
}

void
puglDisplay(PuglView* view)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	if (view->displayFunc) {
		view->displayFunc(view);
	}

	glFlush();
	view->redisplay = false;
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	NSInteger response = [NSApp runModalSession:view->impl->session];
	if (response != NSRunContinuesResponse) {
		if (view->closeFunc) {
			view->closeFunc(view);
		}
	}

	if (view->redisplay) {
		puglDisplay(view);
	}

	return PUGL_SUCCESS;
}

void
puglPostRedisplay(PuglView* view)
{
	view->redisplay = true;
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return (PuglNativeWindow)view->impl->view;
}
