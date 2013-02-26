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

@interface PuglWindow : NSWindow
{
@public
	PuglView* puglview;
}

- (id) initWithContentRect:(NSRect)contentRect
                 styleMask:(unsigned int)aStyle
                   backing:(NSBackingStoreType)bufferingType
                     defer:(BOOL)flag;
- (void) setPuglview:(PuglView*)view;
- (BOOL) windowShouldClose:(id)sender;
- (void) becomeKeyWindow:(id)sender;
- (BOOL) canBecomeKeyWindow:(id)sender;
@end

@implementation PuglWindow

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(unsigned int)aStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag
{
	NSWindow* result = [super initWithContentRect:contentRect
	                                    styleMask:(NSClosableWindowMask |
	                                               NSTitledWindowMask |
	                                               NSResizableWindowMask)
	                                      backing:NSBackingStoreBuffered defer:NO];

	[result setAcceptsMouseMovedEvents:YES];
	[result setLevel: CGShieldingWindowLevel() + 1];

	return result;
}

- (void)setPuglview:(PuglView*)view
{
	puglview = view;
	[self setContentSize:NSMakeSize(view->width, view->height) ];
}

- (BOOL)windowShouldClose:(id)sender
{
	if (puglview->closeFunc)
		puglview->closeFunc(puglview);
	return YES;
}

@end

@interface PuglOpenGLView : NSOpenGLView
{
	int colorBits;
	int depthBits;
@public
	PuglView* puglview;

	NSTrackingArea* trackingArea;
}

- (id) initWithFrame:(NSRect)frame
           colorBits:(int)numColorBits
           depthBits:(int)numDepthBits;
- (void) reshape;
- (void) drawRect:(NSRect)rect;
- (void) mouseMoved:(NSEvent*)event;
- (void) mouseDragged:(NSEvent*)event;
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

	if (puglview) {
		/* NOTE: Apparently reshape gets called when the GC gets around to
		   deleting the view (?), so we must have reset puglview to NULL when
		   this comes around.
		*/
		if (puglview->reshapeFunc) {
			puglview->reshapeFunc(puglview, width, height);
		} else {
			puglDefaultReshape(puglview, width, height);
		}

		puglview->width  = width;
		puglview->height = height;
	}
}

- (void) drawRect:(NSRect)rect
{
	puglDisplay(puglview);
	glFlush();
	glSwapAPPLE();
}

static unsigned
getModifiers(PuglView* view, NSevent* ev)
{
	const unsigned modifierFlags = [ev modifierFlags]

	view->event_timestamp_ms = fmod([ev timestamp] * 1000.0, UINT32_MAX);

	unsigned mods = 0;
	mods |= (modifierFlags & NSShiftKeyMask)     ? PUGL_MOD_SHIFT : 0;
	mods |= (modifierFlags & NSControlKeyMask)   ? PUGL_MOD_CTRL  : 0;
	mods |= (modifierFlags & NSAlternateKeyMask) ? PUGL_MOD_ALT   : 0;
	mods |= (modifierFlags & NSCommandKeyMask)   ? PUGL_MOD_SUPER : 0;
	return mods;
}

-(void)updateTrackingAreas
{
	if (trackingArea != nil) {
		[self removeTrackingArea:trackingArea];
		[trackingArea release];
	}

	const int opts = (NSTrackingMouseEnteredAndExited |
	                  NSTrackingMouseMoved |
	                  NSTrackingActiveAlways);
	trackingArea = [ [NSTrackingArea alloc] initWithRect:[self bounds]
	                                             options:opts
	                                               owner:self
	                                            userInfo:nil];
	[self addTrackingArea:trackingArea];
}

- (void)mouseEntered:(NSEvent*)theEvent
{
	[self updateTrackingAreas];
}

- (void)mouseExited:(NSEvent*)theEvent
{
}

- (void) mouseMoved:(NSEvent*)event
{
	if (puglview->motionFunc) {
		NSPoint loc = [event locationInWindow];
		puglview->mods = getModifiers(puglview, event);
		puglview->motionFunc(puglview, loc.x, puglview->height - loc.y);
	}
}

- (void) mouseDragged:(NSEvent*)event
{
	if (puglview->motionFunc) {
		NSPoint loc = [event locationInWindow];
		puglview->mods = getModifiers(puglview, event);
		puglview->motionFunc(puglview, loc.x, puglview->height - loc.y);
	}
}

- (void) mouseDown:(NSEvent*)event
{
	if (puglview->mouseFunc) {
		NSPoint loc = [event locationInWindow];
		puglview->mods = getModifiers(puglview, event);
		puglview->mouseFunc(puglview, 1, true, loc.x, puglview->height - loc.y);
	}
}

- (void) mouseUp:(NSEvent*)event
{
	if (puglview->mouseFunc) {
		NSPoint loc = [event locationInWindow];
		puglview->mods = getModifiers(puglview, event);
		puglview->mouseFunc(puglview, 1, false, loc.x, puglview->height - loc.y);
	}
	[self updateTrackingAreas];
}

- (void) rightMouseDown:(NSEvent*)event
{
	if (puglview->mouseFunc) {
		NSPoint loc = [event locationInWindow];
		puglview->mods = getModifiers(puglview, event);
		puglview->mouseFunc(puglview, 3, true, loc.x, puglview->height - loc.y);
	}
}

- (void) rightMouseUp:(NSEvent*)event
{
	if (puglview->mouseFunc) {
		NSPoint loc = [event locationInWindow];
		puglview->mods = getModifiers(puglview, event);
		puglview->mouseFunc(puglview, 3, false, loc.x, puglview->height - loc.y);
	}
}

- (void) scrollWheel:(NSEvent*)event
{
	if (puglview->scrollFunc) {
		puglview->mods = getModifiers(puglview, event);
		puglview->scrollFunc(puglview, [event deltaX], [event deltaY]);
	}
	[self updateTrackingAreas];
}

- (void) keyDown:(NSEvent*)event
{
	if (puglview->keyboardFunc && !(puglview->ignoreKeyRepeat && [event isARepeat])) {
		NSString* chars = [event characters];
		puglview->mods = getModifiers(puglview, event);
		puglview->keyboardFunc(puglview, true, [chars characterAtIndex:0]);
	}
}

- (void) keyUp:(NSEvent*)event
{
	if (puglview->keyboardFunc) {
		NSString* chars = [event characters];
		puglview->mods = getModifiers(puglview, event);
		puglview->keyboardFunc(puglview, false,  [chars characterAtIndex:0]);
	}
}

- (void) flagsChanged:(NSEvent*)event
{
	if (puglview->specialFunc) {
		const unsigned mods = getModifiers(puglview, [event modifierFlags]);
		if ((mods & PUGL_MOD_SHIFT) != (puglview->mods & PUGL_MOD_SHIFT)) {
			puglview->specialFunc(puglview, mods & PUGL_MOD_SHIFT, PUGL_KEY_SHIFT);
		} else if ((mods & PUGL_MOD_CTRL) != (puglview->mods & PUGL_MOD_CTRL)) {
			puglview->specialFunc(puglview, mods & PUGL_MOD_CTRL, PUGL_KEY_CTRL);
		} else if ((mods & PUGL_MOD_ALT) != (puglview->mods & PUGL_MOD_ALT)) {
			puglview->specialFunc(puglview, mods & PUGL_MOD_ALT, PUGL_KEY_ALT);
		} else if ((mods & PUGL_MOD_SUPER) != (puglview->mods & PUGL_MOD_SUPER)) {
			puglview->specialFunc(puglview, mods & PUGL_MOD_SUPER, PUGL_KEY_SUPER);
		}
		puglview->mods = mods;
	}
}

@end

struct PuglInternalsImpl {
	PuglOpenGLView* glview;
	id              window;
};

PuglView*
puglCreate(PuglNativeWindow parent,
           const char*      title,
           int              width,
           int              height,
           bool             resizable,
           bool             addToDesktop)
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

	NSString* titleString = [[NSString alloc]
		                        initWithBytes:title
		                               length:strlen(title)
		                             encoding:NSUTF8StringEncoding];

	id window = [[PuglWindow new]retain];

	[window setPuglview:view];
	[window setTitle:titleString];

	impl->glview       = [PuglOpenGLView new];
	impl->window     = window;
	impl->glview->puglview = view;

	[window setContentView:impl->glview];
	[NSApp activateIgnoringOtherApps:YES];
	[window makeFirstResponder:impl->glview];

	[window makeKeyAndOrderFront:window];

	return view;
}

void
puglDestroy(PuglView* view)
{
	view->impl->glview->puglview = NULL;
	[view->impl->window close];
	[view->impl->glview release];
	[view->impl->window release];
	free(view->impl);
	free(view);
}

void
puglDisplay(PuglView* view)
{
	if (view->displayFunc) {
		view->displayFunc(view);
	}
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	[view->impl->glview setNeedsDisplay: YES];

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
	return (PuglNativeWindow)view->impl->glview;
}
