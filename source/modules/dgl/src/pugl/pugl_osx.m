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
- (BOOL) canBecomeKeyWindow;
- (BOOL) windowShouldClose:(id)sender;
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

	return (PuglWindow*)result;

	// unused
	(void)aStyle; (void)bufferingType; (void)flag;
}

- (void)setPuglview:(PuglView*)view
{
	puglview = view;
	[self setContentSize:NSMakeSize(view->width, view->height)];
}

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

- (BOOL)windowShouldClose:(id)sender
{
	if (puglview->closeFunc)
		puglview->closeFunc(puglview);
	return YES;

	// unused
	(void)sender;
}

@end

static void
puglDisplay(PuglView* view)
{
	view->redisplay = false;
	if (view->displayFunc) {
		view->displayFunc(view);
	}
}

@interface PuglOpenGLView : NSOpenGLView
{
@public
	PuglView* puglview;
	NSTrackingArea* trackingArea;
	bool doubleBuffered;
}

- (BOOL) acceptsFirstMouse:(NSEvent*)e;
- (BOOL) acceptsFirstResponder;
- (BOOL) isFlipped;
- (BOOL) isOpaque;
- (BOOL) preservesContentInLiveResize;
- (id)   initWithFrame:(NSRect)frame;
- (void) reshape;
- (void) drawRect:(NSRect)r;
- (void) cursorUpdate:(NSEvent*)e;
- (void) updateTrackingAreas;
- (void) viewWillMoveToWindow:(NSWindow*)newWindow;
- (void) mouseMoved:(NSEvent*)event;
- (void) mouseDragged:(NSEvent*)event;
- (void) rightMouseDragged:(NSEvent*)event;
- (void) otherMouseDragged:(NSEvent*)event;
- (void) mouseDown:(NSEvent*)event;
- (void) rightMouseDown:(NSEvent*)event;
- (void) otherMouseDown:(NSEvent*)event;
- (void) mouseUp:(NSEvent*)event;
- (void) rightMouseUp:(NSEvent*)event;
- (void) otherMouseUp:(NSEvent*)event;
- (void) scrollWheel:(NSEvent*)event;
- (void) keyDown:(NSEvent*)event;
- (void) keyUp:(NSEvent*)event;
- (void) flagsChanged:(NSEvent*)event;

@end

@implementation PuglOpenGLView

- (BOOL) acceptsFirstMouse:(NSEvent*)e
{
	return YES;

	// unused
	(void)e;
}

- (BOOL) acceptsFirstResponder
{
	return YES;
}

- (BOOL) isFlipped
{
	return YES;
}

- (BOOL) isOpaque
{
	return YES;
}

- (BOOL) preservesContentInLiveResize
{
	return NO;
}

- (id) initWithFrame:(NSRect)frame
{
	puglview       = nil;
	trackingArea   = nil;
	doubleBuffered = true;

	NSOpenGLPixelFormatAttribute pixelAttribs[] = {
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 16,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		0
	};

	NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc]
		              initWithAttributes:pixelAttribs];

	if (pixelFormat) {
		self = [super initWithFrame:frame pixelFormat:pixelFormat];
		[pixelFormat release];
		printf("Is doubleBuffered? TRUE\n");
	} else {
		self = [super initWithFrame:frame];
		doubleBuffered = false;
		printf("Is doubleBuffered? FALSE\n");
	}

	if (self) {
		GLint swapInterval = 1;
		[[self openGLContext] setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];

		[self reshape];
	}

	return self;
}

- (void) reshape
{
	[[self openGLContext] update];

	if (!puglview) {
		/* NOTE: Apparently reshape gets called when the GC gets around to
		   deleting the view (?), so we must have reset puglview to NULL when
		   this comes around.
		*/
		return;
	}

	NSRect bounds = [self bounds];
	int    width  = bounds.size.width;
	int    height = bounds.size.height;

	puglEnterContext(puglview);

	if (puglview->reshapeFunc) {
		puglview->reshapeFunc(puglview, width, height);
	} else {
		puglDefaultReshape(puglview, width, height);
	}

	puglLeaveContext(puglview, false);

	puglview->width  = width;
	puglview->height = height;
}

- (void) drawRect:(NSRect)r
{
	puglEnterContext(puglview);
	puglDisplay(puglview);
	puglLeaveContext(puglview, true);

	// unused
	return; (void)r;
}

- (void) cursorUpdate:(NSEvent*)e
{
	[[NSCursor arrowCursor] set];

	// unused
	return; (void)e;
}

- (void) updateTrackingAreas
{
	static const int opts = NSTrackingMouseEnteredAndExited
	                      | NSTrackingMouseMoved
                          | NSTrackingEnabledDuringMouseDrag
                          | NSTrackingInVisibleRect
                          | NSTrackingActiveAlways
                          | NSTrackingCursorUpdate;

	if (trackingArea != nil) {
		[self removeTrackingArea:trackingArea];
		[trackingArea release];
	}

	trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
	                                            options:opts
	                                              owner:self
	                                           userInfo:nil];
	[self addTrackingArea:trackingArea];
	[super updateTrackingAreas];
}

- (void) viewWillMoveToWindow:(NSWindow*)newWindow
{
	if (newWindow != nil) {
		[newWindow setAcceptsMouseMovedEvents:YES];
		[newWindow makeFirstResponder:self];
	}

	[super viewWillMoveToWindow:newWindow];
}

static unsigned
getModifiers(PuglView* view, NSEvent* ev)
{
	const unsigned modifierFlags = [ev modifierFlags];

	view->event_timestamp_ms = fmod([ev timestamp] * 1000.0, UINT32_MAX);

	unsigned mods = 0;
	mods |= (modifierFlags & NSShiftKeyMask)     ? PUGL_MOD_SHIFT : 0;
	mods |= (modifierFlags & NSControlKeyMask)   ? PUGL_MOD_CTRL  : 0;
	mods |= (modifierFlags & NSAlternateKeyMask) ? PUGL_MOD_ALT   : 0;
	mods |= (modifierFlags & NSCommandKeyMask)   ? PUGL_MOD_SUPER : 0;
	return mods;
}

static int
getFixedAppKitButton(NSInteger button)
{
	switch (button) {
	case 0:  return 1;
	case 1:  return 3;
	case 2:  return 2;
	default: return button;
	}
}

- (void) mouseMoved:(NSEvent*)event
{
	if (puglview->motionFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->motionFunc(puglview, loc.x, loc.y);
	}
}

- (void) mouseDragged:(NSEvent*)event
{
	[self mouseMoved:event];
}

- (void) rightMouseDragged:(NSEvent*)event
{
	[self mouseDragged:event];
}

- (void) otherMouseDragged:(NSEvent*)event
{
	[self mouseDragged:event];
}

- (void) mouseDown:(NSEvent*)event
{
	if (puglview->mouseFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->mouseFunc(puglview, getFixedAppKitButton([event buttonNumber]), true, loc.x, loc.y);
	}
}

- (void) rightMouseDown:(NSEvent*)event
{
	[self mouseDown:event];
}

- (void) otherMouseDown:(NSEvent*)event
{
	[self mouseDown:event];
}

- (void) mouseUp:(NSEvent*)event
{
	if (puglview->mouseFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->mouseFunc(puglview, getFixedAppKitButton([event buttonNumber]), false, loc.x, loc.y);
	}
}

- (void) rightMouseUp:(NSEvent*)event
{
	[self mouseUp:event];
}

- (void) otherMouseUp:(NSEvent*)event
{
	[self mouseUp:event];
}

- (void) scrollWheel:(NSEvent*)event
{
	if (puglview->scrollFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->scrollFunc(puglview,
		                     loc.x, loc.y,
		                     [event deltaX], [event deltaY]);
	}
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
		const unsigned mods = getModifiers(puglview, event);
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
		[[view->impl->glview openGLContext] makeCurrentContext];
	}
#endif
}

void
puglLeaveContext(PuglView* view, bool flush)
{
#ifdef PUGL_HAVE_GL
	if (view->ctx_type == PUGL_GL && flush) {
		if (view->impl->glview->doubleBuffered) {
			[[view->impl->glview openGLContext] flushBuffer];
		} else {
			glFlush();
		}
		//[NSOpenGLContext clearCurrentContext];
	}
#endif
}

int
puglCreateWindow(PuglView* view, const char* title)
{
	PuglInternals* impl = view->impl;

	[NSAutoreleasePool new];
	[NSApplication sharedApplication];

	impl->glview = [PuglOpenGLView new];

	if (!impl->glview) {
		return 1;
	}
	
	impl->glview->puglview = view;

	if (view->resizable) {
		[impl->glview setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
	}

	if (view->parent) {
		[impl->glview retain];
		NSView* pview = (NSView*)view->parent;
		[pview addSubview:impl->glview];
	 	return 0;
	}

	id window = [[PuglWindow new]retain];

	if (title) {
		NSString* titleString = [[NSString alloc]
					  initWithBytes:title
					         length:strlen(title)
					       encoding:NSUTF8StringEncoding];

		[window setTitle:titleString];
	}

	[window setPuglview:view];
	[window setContentView:impl->glview];
	[window makeFirstResponder:impl->glview];
	[window makeKeyAndOrderFront:window];

	// wait for first puglShowWindow
	[window setIsVisible:NO];

	[NSApp activateIgnoringOtherApps:YES];
	[window center];

	impl->window = window;

	return 0;
}

void
puglShowWindow(PuglView* view)
{
	PuglInternals* impl = view->impl;

	if (impl->window) {
		[impl->window setIsVisible:YES];
	} else {
		[view->impl->glview setHidden:NO];
	}
}

void
puglHideWindow(PuglView* view)
{
	PuglInternals* impl = view->impl;

	if (impl->window) {
		[impl->window setIsVisible:NO];
	} else {
		[impl->glview setHidden:YES];
	}
}

void
puglDestroy(PuglView* view)
{
	view->impl->glview->puglview = NULL;

	if (view->impl->window) {
		[view->impl->window close];
		[view->impl->glview release];
		[view->impl->window release];
	} else {
		[view->impl->glview release];
	}

	free(view->impl);
	free(view);
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	return PUGL_SUCCESS;

	// unused
	(void)view;
}

void
puglPostRedisplay(PuglView* view)
{
	view->redisplay = true;
	[view->impl->glview setNeedsDisplay:YES];
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return (PuglNativeWindow)view->impl->glview;
}

void*
puglGetContext(PuglView* view)
{
	return NULL;
}
