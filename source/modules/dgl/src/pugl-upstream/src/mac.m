/*
  Copyright 2012-2020 David Robillard <d@drobilla.net>
  Copyright 2017 Hanspeter Portner <dev@open-music-kontrollers.ch>

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

#define GL_SILENCE_DEPRECATION 1

#include "mac.h"

#include "implementation.h"

#include "pugl/pugl.h"

#import <Cocoa/Cocoa.h>

#include <mach/mach_time.h>

#include <stdlib.h>

#ifndef __MAC_10_10
typedef NSUInteger NSEventModifierFlags;
typedef NSUInteger NSEventSubtype;
#endif

#ifndef __MAC_10_12
typedef NSUInteger NSWindowStyleMask;
#endif

static NSRect
rectToScreen(NSScreen* screen, NSRect rect)
{
  const double screenHeight = [screen frame].size.height;

  rect.origin.y = screenHeight - rect.origin.y - rect.size.height;
  return rect;
}

static NSScreen*
viewScreen(PuglView* view)
{
  return view->impl->window ? [view->impl->window screen]
                            : [NSScreen mainScreen];
}

static NSRect
nsRectToPoints(PuglView* view, const NSRect rect)
{
  const double scaleFactor = [viewScreen(view) backingScaleFactor];

  return NSMakeRect(rect.origin.x / scaleFactor,
                    rect.origin.y / scaleFactor,
                    rect.size.width / scaleFactor,
                    rect.size.height / scaleFactor);
}

static NSRect
nsRectFromPoints(PuglView* view, const NSRect rect)
{
  const double scaleFactor = [viewScreen(view) backingScaleFactor];

  return NSMakeRect(rect.origin.x * scaleFactor,
                    rect.origin.y * scaleFactor,
                    rect.size.width * scaleFactor,
                    rect.size.height * scaleFactor);
}

static NSPoint
nsPointFromPoints(PuglView* view, const NSPoint point)
{
  const double scaleFactor = [viewScreen(view) backingScaleFactor];

  return NSMakePoint(point.x * scaleFactor, point.y * scaleFactor);
}

static NSRect
rectToNsRect(const PuglRect rect)
{
  return NSMakeRect(rect.x, rect.y, rect.width, rect.height);
}

static NSSize
sizePoints(PuglView* view, const double width, const double height)
{
  const double scaleFactor = [viewScreen(view) backingScaleFactor];

  return NSMakeSize(width / scaleFactor, height / scaleFactor);
}

static void
updateViewRect(PuglView* view)
{
  NSWindow* const window = view->impl->window;
  if (window) {
    const NSRect screenFramePt = [[NSScreen mainScreen] frame];
    const NSRect screenFramePx = nsRectFromPoints(view, screenFramePt);
    const NSRect framePt       = [window frame];
    const NSRect contentPt     = [window contentRectForFrameRect:framePt];
    const NSRect contentPx     = nsRectFromPoints(view, contentPt);
    const double screenHeight  = screenFramePx.size.height;

    view->frame.x = contentPx.origin.x;
    view->frame.y = screenHeight - contentPx.origin.y - contentPx.size.height;
    view->frame.width  = contentPx.size.width;
    view->frame.height = contentPx.size.height;
  }
}

@implementation PuglWindow {
@public
  PuglView* puglview;
}

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(NSWindowStyleMask)aStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag
{
  (void)flag;

  NSWindow* result = [super initWithContentRect:contentRect
                                      styleMask:aStyle
                                        backing:bufferingType
                                          defer:NO];

  [result setAcceptsMouseMovedEvents:YES];
  return (PuglWindow*)result;
}

- (void)setPuglview:(PuglView*)view
{
  puglview = view;

  [self setContentSize:sizePoints(view, view->frame.width, view->frame.height)];
}

- (BOOL)canBecomeKeyWindow
{
  return YES;
}

- (BOOL)canBecomeMainWindow
{
  return YES;
}

- (void)setIsVisible:(BOOL)flag
{
  if (flag && !puglview->visible) {
    const PuglConfigureEvent ev = {
      PUGL_CONFIGURE,
      0,
      puglview->frame.x,
      puglview->frame.y,
      puglview->frame.width,
      puglview->frame.height,
    };

    PuglEvent configureEvent;
    configureEvent.configure = ev;
    puglDispatchEvent(puglview, &configureEvent);
    puglDispatchSimpleEvent(puglview, PUGL_MAP);
  } else if (!flag && puglview->visible) {
    puglDispatchSimpleEvent(puglview, PUGL_UNMAP);
  }

  puglview->visible = flag;

  [super setIsVisible:flag];
}

@end

@implementation PuglWrapperView {
@public
  PuglView*                  puglview;
  NSTrackingArea*            trackingArea;
  NSMutableAttributedString* markedText;
  NSMutableDictionary*       userTimers;
  bool                       reshaped;
}

- (void)dispatchExpose:(NSRect)rect
{
  const double scaleFactor = [[NSScreen mainScreen] backingScaleFactor];

  if (reshaped) {
    updateViewRect(puglview);

    const PuglConfigureEvent ev = {
      PUGL_CONFIGURE,
      0,
      puglview->frame.x,
      puglview->frame.y,
      puglview->frame.width,
      puglview->frame.height,
    };

    PuglEvent configureEvent;
    configureEvent.configure = ev;
    puglDispatchEvent(puglview, &configureEvent);
    reshaped = false;
  }

  if (![[puglview->impl->drawView window] isVisible]) {
    return;
  }

  const PuglExposeEvent ev = {
    PUGL_EXPOSE,
    0,
    rect.origin.x * scaleFactor,
    rect.origin.y * scaleFactor,
    rect.size.width * scaleFactor,
    rect.size.height * scaleFactor,
  };

  PuglEvent exposeEvent;
  exposeEvent.expose = ev;
  puglDispatchEvent(puglview, &exposeEvent);
}

- (NSSize)intrinsicContentSize
{
  if (puglview->defaultWidth || puglview->defaultHeight) {
    return sizePoints(
      puglview, puglview->defaultWidth, puglview->defaultHeight);
  }

  return NSMakeSize(NSViewNoInstrinsicMetric, NSViewNoInstrinsicMetric);
}

- (BOOL)isFlipped
{
  return YES;
}

- (BOOL)acceptsFirstResponder
{
  return YES;
}

- (void)setReshaped
{
  reshaped = true;
}

static uint32_t
getModifiers(const NSEvent* const ev)
{
  const NSEventModifierFlags modifierFlags = [ev modifierFlags];

  return (((modifierFlags & NSShiftKeyMask) ? PUGL_MOD_SHIFT : 0) |
          ((modifierFlags & NSControlKeyMask) ? PUGL_MOD_CTRL : 0) |
          ((modifierFlags & NSAlternateKeyMask) ? PUGL_MOD_ALT : 0) |
          ((modifierFlags & NSCommandKeyMask) ? PUGL_MOD_SUPER : 0));
}

static PuglKey
keySymToSpecial(const NSEvent* const ev)
{
  NSString* chars = [ev charactersIgnoringModifiers];
  if ([chars length] == 1) {
    switch ([chars characterAtIndex:0]) {
    case NSF1FunctionKey:
      return PUGL_KEY_F1;
    case NSF2FunctionKey:
      return PUGL_KEY_F2;
    case NSF3FunctionKey:
      return PUGL_KEY_F3;
    case NSF4FunctionKey:
      return PUGL_KEY_F4;
    case NSF5FunctionKey:
      return PUGL_KEY_F5;
    case NSF6FunctionKey:
      return PUGL_KEY_F6;
    case NSF7FunctionKey:
      return PUGL_KEY_F7;
    case NSF8FunctionKey:
      return PUGL_KEY_F8;
    case NSF9FunctionKey:
      return PUGL_KEY_F9;
    case NSF10FunctionKey:
      return PUGL_KEY_F10;
    case NSF11FunctionKey:
      return PUGL_KEY_F11;
    case NSF12FunctionKey:
      return PUGL_KEY_F12;
    case NSDeleteCharacter:
      return PUGL_KEY_BACKSPACE;
    case NSDeleteFunctionKey:
      return PUGL_KEY_DELETE;
    case NSLeftArrowFunctionKey:
      return PUGL_KEY_LEFT;
    case NSUpArrowFunctionKey:
      return PUGL_KEY_UP;
    case NSRightArrowFunctionKey:
      return PUGL_KEY_RIGHT;
    case NSDownArrowFunctionKey:
      return PUGL_KEY_DOWN;
    case NSPageUpFunctionKey:
      return PUGL_KEY_PAGE_UP;
    case NSPageDownFunctionKey:
      return PUGL_KEY_PAGE_DOWN;
    case NSHomeFunctionKey:
      return PUGL_KEY_HOME;
    case NSEndFunctionKey:
      return PUGL_KEY_END;
    case NSInsertFunctionKey:
      return PUGL_KEY_INSERT;
    case NSMenuFunctionKey:
      return PUGL_KEY_MENU;
    case NSScrollLockFunctionKey:
      return PUGL_KEY_SCROLL_LOCK;
    case NSClearLineFunctionKey:
      return PUGL_KEY_NUM_LOCK;
    case NSPrintScreenFunctionKey:
      return PUGL_KEY_PRINT_SCREEN;
    case NSPauseFunctionKey:
      return PUGL_KEY_PAUSE;
    }
    // SHIFT, CTRL, ALT, and SUPER are handled in [flagsChanged]
  }
  return (PuglKey)0;
}

- (void)updateTrackingAreas
{
  if (trackingArea != nil) {
    [self removeTrackingArea:trackingArea];
    [trackingArea release];
  }

  const int opts = (NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                    NSTrackingActiveAlways);
  trackingArea   = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                              options:opts
                                                owner:self
                                             userInfo:nil];
  [self addTrackingArea:trackingArea];
  [super updateTrackingAreas];
}

- (NSPoint)eventLocation:(NSEvent*)event
{
  return nsPointFromPoints(
    puglview, [self convertPoint:[event locationInWindow] fromView:nil]);
}

static void
handleCrossing(PuglWrapperView* view, NSEvent* event, const PuglEventType type)
{
  const NSPoint           wloc = [view eventLocation:event];
  const NSPoint           rloc = [NSEvent mouseLocation];
  const PuglCrossingEvent ev   = {
    type,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
    PUGL_CROSSING_NORMAL,
  };

  PuglEvent crossingEvent;
  crossingEvent.crossing = ev;
  puglDispatchEvent(view->puglview, &crossingEvent);
}

- (void)mouseEntered:(NSEvent*)event
{
  handleCrossing(self, event, PUGL_POINTER_IN);
  [puglview->impl->cursor set];
  puglview->impl->mouseTracked = true;
}

- (void)mouseExited:(NSEvent*)event
{
  [[NSCursor arrowCursor] set];
  handleCrossing(self, event, PUGL_POINTER_OUT);
  puglview->impl->mouseTracked = false;
}

- (void)cursorUpdate:(NSEvent*)event
{
  (void)event;
  [puglview->impl->cursor set];
}

- (void)mouseMoved:(NSEvent*)event
{
  const NSPoint         wloc = [self eventLocation:event];
  const NSPoint         rloc = [NSEvent mouseLocation];
  const PuglMotionEvent ev   = {
    PUGL_MOTION,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
  };

  PuglEvent motionEvent;
  motionEvent.motion = ev;
  puglDispatchEvent(puglview, &motionEvent);
}

- (void)mouseDragged:(NSEvent*)event
{
  [self mouseMoved:event];
}

- (void)rightMouseDragged:(NSEvent*)event
{
  [self mouseMoved:event];
}

- (void)otherMouseDragged:(NSEvent*)event
{
  [self mouseMoved:event];
}

- (void)mouseDown:(NSEvent*)event
{
  const NSPoint         wloc = [self eventLocation:event];
  const NSPoint         rloc = [NSEvent mouseLocation];
  const PuglButtonEvent ev   = {
    PUGL_BUTTON_PRESS,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
    (uint32_t)[event buttonNumber] + 1,
  };

  PuglEvent pressEvent;
  pressEvent.button = ev;
  puglDispatchEvent(puglview, &pressEvent);
}

- (void)mouseUp:(NSEvent*)event
{
  const NSPoint         wloc = [self eventLocation:event];
  const NSPoint         rloc = [NSEvent mouseLocation];
  const PuglButtonEvent ev   = {
    PUGL_BUTTON_RELEASE,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
    (uint32_t)[event buttonNumber] + 1,
  };

  PuglEvent releaseEvent;
  releaseEvent.button = ev;
  puglDispatchEvent(puglview, &releaseEvent);
}

- (void)rightMouseDown:(NSEvent*)event
{
  [self mouseDown:event];
}

- (void)rightMouseUp:(NSEvent*)event
{
  [self mouseUp:event];
}

- (void)otherMouseDown:(NSEvent*)event
{
  [self mouseDown:event];
}

- (void)otherMouseUp:(NSEvent*)event
{
  [self mouseUp:event];
}

- (void)scrollWheel:(NSEvent*)event
{
  const NSPoint             wloc = [self eventLocation:event];
  const NSPoint             rloc = [NSEvent mouseLocation];
  const double              dx   = [event scrollingDeltaX];
  const double              dy   = [event scrollingDeltaY];
  const PuglScrollDirection dir =
    ((dx == 0.0 && dy > 0.0)
       ? PUGL_SCROLL_UP
       : ((dx == 0.0 && dy < 0.0)
            ? PUGL_SCROLL_DOWN
            : ((dy == 0.0 && dx > 0.0)
                 ? PUGL_SCROLL_RIGHT
                 : ((dy == 0.0 && dx < 0.0) ? PUGL_SCROLL_LEFT
                                            : PUGL_SCROLL_SMOOTH))));

  const PuglScrollEvent ev = {
    PUGL_SCROLL,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
    [event hasPreciseScrollingDeltas] ? PUGL_SCROLL_SMOOTH : dir,
    dx,
    dy,
  };

  PuglEvent scrollEvent;
  scrollEvent.scroll = ev;
  puglDispatchEvent(puglview, &scrollEvent);
}

- (void)keyDown:(NSEvent*)event
{
  if (puglview->hints[PUGL_IGNORE_KEY_REPEAT] && [event isARepeat]) {
    return;
  }

  const NSPoint   wloc  = [self eventLocation:event];
  const NSPoint   rloc  = [NSEvent mouseLocation];
  const PuglKey   spec  = keySymToSpecial(event);
  const NSString* chars = [event charactersIgnoringModifiers];
  const char*     str   = [[chars lowercaseString] UTF8String];
  const uint32_t  code  = (spec ? spec : puglDecodeUTF8((const uint8_t*)str));

  const PuglKeyEvent ev = {
    PUGL_KEY_PRESS,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
    [event keyCode],
    (code != 0xFFFD) ? code : 0,
  };

  PuglEvent pressEvent;
  pressEvent.key = ev;
  puglDispatchEvent(puglview, &pressEvent);

  if (!spec) {
    [self interpretKeyEvents:@[event]];
  }
}

- (void)keyUp:(NSEvent*)event
{
  const NSPoint   wloc  = [self eventLocation:event];
  const NSPoint   rloc  = [NSEvent mouseLocation];
  const PuglKey   spec  = keySymToSpecial(event);
  const NSString* chars = [event charactersIgnoringModifiers];
  const char*     str   = [[chars lowercaseString] UTF8String];
  const uint32_t  code  = (spec ? spec : puglDecodeUTF8((const uint8_t*)str));

  const PuglKeyEvent ev = {
    PUGL_KEY_RELEASE,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
    [event keyCode],
    (code != 0xFFFD) ? code : 0,
  };

  PuglEvent releaseEvent;
  releaseEvent.key = ev;
  puglDispatchEvent(puglview, &releaseEvent);
}

- (BOOL)hasMarkedText
{
  return [markedText length] > 0;
}

- (NSRange)markedRange
{
  return (([markedText length] > 0) ? NSMakeRange(0, [markedText length] - 1)
                                    : NSMakeRange(NSNotFound, 0));
}

- (NSRange)selectedRange
{
  return NSMakeRange(NSNotFound, 0);
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selected
     replacementRange:(NSRange)replacement
{
  (void)selected;
  (void)replacement;
  [markedText release];
  markedText =
    ([(NSObject*)string isKindOfClass:[NSAttributedString class]]
       ? [[NSMutableAttributedString alloc] initWithAttributedString:string]
       : [[NSMutableAttributedString alloc] initWithString:string]);
}

- (void)unmarkText
{
  [[markedText mutableString] setString:@""];
}

- (NSArray*)validAttributesForMarkedText
{
  return @[];
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:
                                                 (NSRangePointer)actual
{
  (void)range;
  (void)actual;
  return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
  (void)point;
  return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range
                         actualRange:(NSRangePointer)actual
{
  (void)range;
  (void)actual;

  const NSRect frame = [self bounds];
  return NSMakeRect(frame.origin.x, frame.origin.y, 0.0, 0.0);
}

- (void)doCommandBySelector:(SEL)selector
{
  (void)selector;
}

- (void)insertText:(id)string replacementRange:(NSRange)replacement
{
  (void)replacement;

  NSEvent* const  event = [NSApp currentEvent];
  NSString* const characters =
    ([(NSObject*)string isKindOfClass:[NSAttributedString class]]
       ? [(NSAttributedString*)string string]
       : (NSString*)string);

  const NSPoint wloc = [self eventLocation:event];
  const NSPoint rloc = [NSEvent mouseLocation];
  for (size_t i = 0; i < [characters length]; ++i) {
    const uint32_t code    = [characters characterAtIndex:i];
    char           utf8[8] = {0};
    NSUInteger     len     = 0;

    [characters getBytes:utf8
               maxLength:sizeof(utf8)
              usedLength:&len
                encoding:NSUTF8StringEncoding
                 options:0
                   range:NSMakeRange(i, i + 1)
          remainingRange:nil];

    PuglTextEvent ev = {
      PUGL_TEXT,
      0,
      [event timestamp],
      wloc.x,
      wloc.y,
      rloc.x,
      [[NSScreen mainScreen] frame].size.height - rloc.y,
      getModifiers(event),
      [event keyCode],
      code,
      { 0, 0, 0, 0, 0, 0, 0, 0 },
    };

    memcpy(ev.string, utf8, len);

    PuglEvent textEvent;
    textEvent.text = ev;
    puglDispatchEvent(puglview, &textEvent);
  }
}

- (void)flagsChanged:(NSEvent*)event
{
  const uint32_t mods    = getModifiers(event);
  PuglEventType  type    = PUGL_NOTHING;
  PuglKey        special = (PuglKey)0;

  if ((mods & PUGL_MOD_SHIFT) != (puglview->impl->mods & PUGL_MOD_SHIFT)) {
    type    = mods & PUGL_MOD_SHIFT ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
    special = PUGL_KEY_SHIFT;
  } else if ((mods & PUGL_MOD_CTRL) != (puglview->impl->mods & PUGL_MOD_CTRL)) {
    type    = mods & PUGL_MOD_CTRL ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
    special = PUGL_KEY_CTRL;
  } else if ((mods & PUGL_MOD_ALT) != (puglview->impl->mods & PUGL_MOD_ALT)) {
    type    = mods & PUGL_MOD_ALT ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
    special = PUGL_KEY_ALT;
  } else if ((mods & PUGL_MOD_SUPER) !=
             (puglview->impl->mods & PUGL_MOD_SUPER)) {
    type    = mods & PUGL_MOD_SUPER ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
    special = PUGL_KEY_SUPER;
  }

  if (special != 0) {
    const NSPoint wloc = [self eventLocation:event];
    const NSPoint rloc = [NSEvent mouseLocation];

    const PuglKeyEvent ev = {type,
                             0,
                             [event timestamp],
                             wloc.x,
                             wloc.y,
                             rloc.x,
                             [[NSScreen mainScreen] frame].size.height - rloc.y,
                             mods,
                             [event keyCode],
                             special};

    PuglEvent keyEvent;
    keyEvent.key = ev;
    puglDispatchEvent(puglview, &keyEvent);
  }

  puglview->impl->mods = mods;
}

- (BOOL)preservesContentInLiveResize
{
  return NO;
}

- (void)viewWillStartLiveResize
{
  puglDispatchSimpleEvent(puglview, PUGL_LOOP_ENTER);
}

- (void)viewWillDraw
{
  puglDispatchSimpleEvent(puglview, PUGL_UPDATE);
  [super viewWillDraw];
}

- (void)resizeTick
{
  puglPostRedisplay(puglview);
}

- (void)timerTick:(NSTimer*)userTimer
{
  const NSNumber*      userInfo = userTimer.userInfo;
  const PuglTimerEvent ev       = {PUGL_TIMER, 0, userInfo.unsignedLongValue};

  PuglEvent timerEvent;
  timerEvent.timer = ev;
  puglDispatchEvent(puglview, &timerEvent);
}

- (void)viewDidEndLiveResize
{
  puglDispatchSimpleEvent(puglview, PUGL_LOOP_LEAVE);
}

@end

@interface PuglWindowDelegate : NSObject<NSWindowDelegate>

- (instancetype)initWithPuglWindow:(PuglWindow*)window;

@end

@implementation PuglWindowDelegate {
  PuglWindow* window;
}

- (instancetype)initWithPuglWindow:(PuglWindow*)puglWindow
{
  if ((self = [super init])) {
    window = puglWindow;
  }

  return self;
}

- (BOOL)windowShouldClose:(id)sender
{
  (void)sender;

  puglDispatchSimpleEvent(window->puglview, PUGL_CLOSE);
  return YES;
}

- (void)windowDidMove:(NSNotification*)notification
{
  (void)notification;

  updateViewRect(window->puglview);
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
  (void)notification;

  PuglEvent ev  = {{PUGL_FOCUS_IN, 0}};
  ev.focus.mode = PUGL_CROSSING_NORMAL;
  puglDispatchEvent(window->puglview, &ev);
}

- (void)windowDidResignKey:(NSNotification*)notification
{
  (void)notification;

  PuglEvent ev  = {{PUGL_FOCUS_OUT, 0}};
  ev.focus.mode = PUGL_CROSSING_NORMAL;
  puglDispatchEvent(window->puglview, &ev);
}

@end

PuglWorldInternals*
puglInitWorldInternals(PuglWorldType type, PuglWorldFlags PUGL_UNUSED(flags))
{
  PuglWorldInternals* impl =
    (PuglWorldInternals*)calloc(1, sizeof(PuglWorldInternals));

  impl->app = [NSApplication sharedApplication];

  if (type == PUGL_PROGRAM) {
    impl->autoreleasePool = [NSAutoreleasePool new];
  }

  return impl;
}

void
puglFreeWorldInternals(PuglWorld* world)
{
  if (world->impl->autoreleasePool) {
    [world->impl->autoreleasePool drain];
  }

  free(world->impl);
}

void*
puglGetNativeWorld(PuglWorld* world)
{
  return world->impl->app;
}

PuglInternals*
puglInitViewInternals(void)
{
  PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));

  impl->cursor = [NSCursor arrowCursor];

  return impl;
}

static NSLayoutConstraint*
puglConstraint(id item, NSLayoutAttribute attribute, float constant)
{
  return
    [NSLayoutConstraint constraintWithItem:item
                                 attribute:attribute
                                 relatedBy:NSLayoutRelationGreaterThanOrEqual
                                    toItem:nil
                                 attribute:NSLayoutAttributeNotAnAttribute
                                multiplier:1.0
                                  constant:(CGFloat)constant];
}

PuglStatus
puglRealize(PuglView* view)
{
  PuglInternals* impl = view->impl;
  if (impl->wrapperView) {
    return PUGL_FAILURE;
  }

  const NSScreen* const screen      = [NSScreen mainScreen];
  const double          scaleFactor = [screen backingScaleFactor];

  // Getting depth from the display mode seems tedious, just set usual values
  if (view->hints[PUGL_RED_BITS] == PUGL_DONT_CARE) {
    view->hints[PUGL_RED_BITS] = 8;
  }
  if (view->hints[PUGL_BLUE_BITS] == PUGL_DONT_CARE) {
    view->hints[PUGL_BLUE_BITS] = 8;
  }
  if (view->hints[PUGL_GREEN_BITS] == PUGL_DONT_CARE) {
    view->hints[PUGL_GREEN_BITS] = 8;
  }
  if (view->hints[PUGL_ALPHA_BITS] == PUGL_DONT_CARE) {
    view->hints[PUGL_ALPHA_BITS] = 8;
  }

  CGDirectDisplayID displayId = CGMainDisplayID();
  CGDisplayModeRef  mode      = CGDisplayCopyDisplayMode(displayId);

  // Try to get refresh rate from mode (usually fails)
  view->hints[PUGL_REFRESH_RATE] = (int)CGDisplayModeGetRefreshRate(mode);

  CGDisplayModeRelease(mode);
  if (view->hints[PUGL_REFRESH_RATE] == 0) {
    // Get refresh rate from a display link
    // TODO: Keep and actually use the display link for something?
    CVDisplayLinkRef link;
    CVDisplayLinkCreateWithCGDisplay(displayId, &link);

    const CVTime p = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(link);
    const double r = p.timeScale / (double)p.timeValue;
    view->hints[PUGL_REFRESH_RATE] = (int)lrint(r);

    CVDisplayLinkRelease(link);
  }

  if (view->frame.width == 0.0 && view->frame.height == 0.0) {
    if (view->defaultWidth == 0.0 && view->defaultHeight == 0.0) {
      return PUGL_BAD_CONFIGURATION;
    }

    const double screenWidthPx  = [screen frame].size.width * scaleFactor;
    const double screenHeightPx = [screen frame].size.height * scaleFactor;

    view->frame.width  = view->defaultWidth;
    view->frame.height = view->defaultHeight;
    view->frame.x      = screenWidthPx / 2.0 - view->frame.width / 2.0;
    view->frame.y      = screenHeightPx / 2.0 - view->frame.height / 2.0;
  }

  const NSRect framePx = rectToNsRect(view->frame);
  const NSRect framePt = NSMakeRect(framePx.origin.x / scaleFactor,
                                    framePx.origin.y / scaleFactor,
                                    framePx.size.width / scaleFactor,
                                    framePx.size.height / scaleFactor);

  // Create wrapper view to handle input
  impl->wrapperView             = [PuglWrapperView alloc];
  impl->wrapperView->puglview   = view;
  impl->wrapperView->userTimers = [[NSMutableDictionary alloc] init];
  impl->wrapperView->markedText = [[NSMutableAttributedString alloc] init];
  [impl->wrapperView setAutoresizesSubviews:YES];
  [impl->wrapperView initWithFrame:framePt];
  [impl->wrapperView addConstraint:puglConstraint(impl->wrapperView,
                                                  NSLayoutAttributeWidth,
                                                  view->minWidth)];
  [impl->wrapperView addConstraint:puglConstraint(impl->wrapperView,
                                                  NSLayoutAttributeHeight,
                                                  view->minHeight)];

  // Create draw view to be rendered to
  PuglStatus st = PUGL_SUCCESS;
  if ((st = view->backend->configure(view)) ||
      (st = view->backend->create(view))) {
    return st;
  }

  // Add draw view to wrapper view
  [impl->wrapperView addSubview:impl->drawView];
  [impl->wrapperView setHidden:NO];
  [impl->drawView setHidden:NO];

  if (view->parent) {
    NSView* pview = (NSView*)view->parent;
    [pview addSubview:impl->wrapperView];
    [impl->drawView setHidden:NO];
    [[impl->drawView window] makeFirstResponder:impl->wrapperView];
  } else {
    unsigned style =
      (NSClosableWindowMask | NSTitledWindowMask | NSMiniaturizableWindowMask);
    if (view->hints[PUGL_RESIZABLE]) {
      style |= NSResizableWindowMask;
    }

    PuglWindow* window = [[[PuglWindow alloc]
      initWithContentRect:rectToScreen([NSScreen mainScreen], framePt)
                styleMask:style
                  backing:NSBackingStoreBuffered
                    defer:NO] retain];
    [window setPuglview:view];

    if (view->title) {
      NSString* titleString =
        [[NSString alloc] initWithBytes:view->title
                                 length:strlen(view->title)
                               encoding:NSUTF8StringEncoding];

      [window setTitle:titleString];
    }

    if (view->minWidth || view->minHeight) {
      [window
        setContentMinSize:sizePoints(view, view->minWidth, view->minHeight)];
    }
    impl->window = window;

    ((NSWindow*)window).delegate =
      [[PuglWindowDelegate alloc] initWithPuglWindow:window];

    if (view->minAspectX && view->minAspectY) {
      [window setContentAspectRatio:sizePoints(view,
                                               view->minAspectX,
                                               view->minAspectY)];
    }

    puglSetFrame(view, view->frame);

    [window setContentView:impl->wrapperView];
    [window makeFirstResponder:impl->wrapperView];
    [window setIsVisible:NO];
  }

  [impl->wrapperView updateTrackingAreas];

  puglDispatchSimpleEvent(view, PUGL_CREATE);

  return PUGL_SUCCESS;
}

PuglStatus
puglShow(PuglView* view)
{
  if (!view->impl->wrapperView) {
    const PuglStatus st = puglRealize(view);
    if (st) {
      return st;
    }
  }

  if (![view->impl->window isVisible]) {
    [view->impl->window setIsVisible:YES];
    [view->impl->drawView setNeedsDisplay:YES];
    updateViewRect(view);
  }

  if (! view->transientParent) {
    [view->impl->window makeKeyAndOrderFront:view->impl->window];
    [view->world->impl->app activateIgnoringOtherApps:YES];
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglHide(PuglView* view)
{
  [view->impl->window setIsVisible:NO];
  return PUGL_SUCCESS;
}

void
puglFreeViewInternals(PuglView* view)
{
  if (view) {
    if (view->backend) {
      view->backend->destroy(view);
    }

    if (view->impl) {
      if (view->impl->wrapperView) {
        [view->impl->wrapperView removeFromSuperview];
        view->impl->wrapperView->puglview = NULL;
      }

      if (view->impl->window) {
        [view->impl->window close];
      }

      if (view->impl->wrapperView) {
        [view->impl->wrapperView release];
      }

      if (view->impl->window) {
        [view->impl->window release];
      }

      free(view->impl);
    }
  }
}

PuglStatus
puglGrabFocus(PuglView* view)
{
  NSWindow* window = [view->impl->wrapperView window];

  [window makeKeyWindow];
  [window makeFirstResponder:view->impl->wrapperView];
  return PUGL_SUCCESS;
}

bool
puglHasFocus(const PuglView* view)
{
  PuglInternals* const impl = view->impl;

  return ([[impl->wrapperView window] isKeyWindow] &&
          [[impl->wrapperView window] firstResponder] == impl->wrapperView);
}

PuglStatus
puglRequestAttention(PuglView* view)
{
  if (![view->impl->window isKeyWindow]) {
    [view->world->impl->app requestUserAttention:NSInformationalRequest];
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglStartTimer(PuglView* view, uintptr_t id, double timeout)
{
  puglStopTimer(view, id);

  NSNumber* idNumber = [NSNumber numberWithUnsignedLong:id];

  NSTimer* timer = [NSTimer timerWithTimeInterval:timeout
                                           target:view->impl->wrapperView
                                         selector:@selector(timerTick:)
                                         userInfo:idNumber
                                          repeats:YES];

  [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];

  view->impl->wrapperView->userTimers[idNumber] = timer;

  return PUGL_SUCCESS;
}

PuglStatus
puglStopTimer(PuglView* view, uintptr_t id)
{
  NSNumber* idNumber = [NSNumber numberWithUnsignedLong:id];
  NSTimer*  timer    = view->impl->wrapperView->userTimers[idNumber];

  if (timer) {
    [view->impl->wrapperView->userTimers removeObjectForKey:timer];
    [timer invalidate];
    return PUGL_SUCCESS;
  }

  return PUGL_UNKNOWN_ERROR;
}

PuglStatus
puglSendEvent(PuglView* view, const PuglEvent* event)
{
  if (event->type == PUGL_CLIENT) {
    PuglEvent copiedEvent = *event;

    CFRunLoopObserverRef observer = CFRunLoopObserverCreateWithHandler(
      NULL,
      kCFRunLoopAfterWaiting,
      false,
      0,
      ^(CFRunLoopObserverRef, CFRunLoopActivity) {
        puglDispatchEvent(view, &copiedEvent);
      });

    CFRunLoopAddObserver(CFRunLoopGetMain(), observer, kCFRunLoopCommonModes);

    CFRelease(observer);

    return PUGL_SUCCESS;
  }

  return PUGL_UNSUPPORTED_TYPE;
}

#ifndef PUGL_DISABLE_DEPRECATED
PuglStatus
puglWaitForEvent(PuglView* view)
{
  return puglPollEvents(view->world, -1.0);
}
#endif

PuglStatus
puglUpdate(PuglWorld* world, const double timeout)
{
  @autoreleasepool {
    if (world->impl->autoreleasePool != nil) {
      NSDate* date =
        ((timeout < 0) ? [NSDate distantFuture]
                       : [NSDate dateWithTimeIntervalSinceNow:timeout]);

      for (NSEvent* ev = NULL;
           (ev = [world->impl->app nextEventMatchingMask:NSAnyEventMask
                                               untilDate:date
                                                  inMode:NSDefaultRunLoopMode
                                                 dequeue:YES]);) {
        [world->impl->app sendEvent:ev];

        if (timeout < 0) {
          // Now that we've waited and got an event, set the date to now to
          // avoid looping forever
          date = [NSDate date];
        }
      }
    }

    for (size_t i = 0; i < world->numViews; ++i) {
      PuglView* const view = world->views[i];

      if ([[view->impl->drawView window] isVisible]) {
        puglDispatchSimpleEvent(view, PUGL_UPDATE);
      }

      [view->impl->drawView displayIfNeeded];
    }
  }

  return PUGL_SUCCESS;
}

#ifndef PUGL_DISABLE_DEPRECATED
PuglStatus
puglProcessEvents(PuglView* view)
{
  return puglDispatchEvents(view->world);
}
#endif

double
puglGetTime(const PuglWorld* world)
{
  return (mach_absolute_time() / 1e9) - world->startTime;
}

PuglStatus
puglPostRedisplay(PuglView* view)
{
  [view->impl->drawView setNeedsDisplay:YES];
  return PUGL_SUCCESS;
}

PuglStatus
puglPostRedisplayRect(PuglView* view, const PuglRect rect)
{
  const NSRect rectPx = rectToNsRect(rect);

  [view->impl->drawView setNeedsDisplayInRect:nsRectToPoints(view, rectPx)];

  return PUGL_SUCCESS;
}

PuglNativeView
puglGetNativeWindow(PuglView* view)
{
  return (PuglNativeView)view->impl->wrapperView;
}

PuglStatus
puglSetWindowTitle(PuglView* view, const char* title)
{
  puglSetString(&view->title, title);

  if (view->impl->window) {
    NSString* titleString =
      [[NSString alloc] initWithBytes:title
                               length:strlen(title)
                             encoding:NSUTF8StringEncoding];

    [view->impl->window setTitle:titleString];
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglSetFrame(PuglView* view, const PuglRect frame)
{
  PuglInternals* const impl = view->impl;

  // Update view frame to exactly the requested frame in Pugl coordinates
  view->frame = frame;

  const NSRect framePx = rectToNsRect(frame);
  const NSRect framePt = nsRectToPoints(view, framePx);
  if (impl->window) {
    // Resize window to fit new content rect
    const NSRect screenPt = rectToScreen(viewScreen(view), framePt);
    const NSRect winFrame = [impl->window frameRectForContentRect:screenPt];

    [impl->window setFrame:winFrame display:NO];
  }

  // Resize views
  const NSRect sizePx = NSMakeRect(0, 0, frame.width, frame.height);
  const NSRect sizePt = [impl->drawView convertRectFromBacking:sizePx];

  [impl->wrapperView setFrame:(impl->window ? sizePt : framePt)];
  [impl->drawView setFrame:sizePt];

  return PUGL_SUCCESS;
}

PuglStatus
puglSetDefaultSize(PuglView* const view, const int width, const int height)
{
  view->defaultWidth  = width;
  view->defaultHeight = height;
  return PUGL_SUCCESS;
}

PuglStatus
puglSetMinSize(PuglView* const view, const int width, const int height)
{
  view->minWidth  = width;
  view->minHeight = height;

  if (view->impl->window && (view->minWidth || view->minHeight)) {
    [view->impl->window
      setContentMinSize:sizePoints(view, view->minWidth, view->minHeight)];
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglSetMaxSize(PuglView* const view, const int width, const int height)
{
  view->maxWidth  = width;
  view->maxHeight = height;

  if (view->impl->window && (view->maxWidth || view->maxHeight)) {
    [view->impl->window
      setContentMaxSize:sizePoints(view, view->maxWidth, view->maxHeight)];
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglSetAspectRatio(PuglView* const view,
                   const int       minX,
                   const int       minY,
                   const int       maxX,
                   const int       maxY)
{
  view->minAspectX = minX;
  view->minAspectY = minY;
  view->maxAspectX = maxX;
  view->maxAspectY = maxY;

  if (view->impl->window && view->minAspectX && view->minAspectY) {
    [view->impl->window setContentAspectRatio:sizePoints(view,
                                                         view->minAspectX,
                                                         view->minAspectY)];
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglSetTransientFor(PuglView* view, PuglNativeView parent)
{
  view->transientParent = parent;

  if (view->impl->window) {
    NSWindow* parentWindow = [(NSView*)parent window];
    if (parentWindow) {
      [parentWindow addChildWindow:view->impl->window ordered:NSWindowAbove];
      return PUGL_SUCCESS;
    }
  }

  return PUGL_FAILURE;
}

const void*
puglGetClipboard(PuglView* const    view,
                 const char** const type,
                 size_t* const      len)
{
  NSPasteboard* const pasteboard = [NSPasteboard generalPasteboard];

  if ([[pasteboard types] containsObject:NSStringPboardType]) {
    const NSString* str  = [pasteboard stringForType:NSStringPboardType];
    const char*     utf8 = [str UTF8String];

    puglSetBlob(&view->clipboard, utf8, strlen(utf8) + 1);
  }

  return puglGetInternalClipboard(view, type, len);
}

static NSCursor*
puglGetNsCursor(const PuglCursor cursor)
{
  SEL cursorSelector = nil;

  switch (cursor) {
  case PUGL_CURSOR_ARROW:
    return [NSCursor arrowCursor];
  case PUGL_CURSOR_CARET:
    return [NSCursor IBeamCursor];
  case PUGL_CURSOR_CROSSHAIR:
    return [NSCursor crosshairCursor];
  case PUGL_CURSOR_HAND:
    return [NSCursor pointingHandCursor];
  case PUGL_CURSOR_NO:
    return [NSCursor operationNotAllowedCursor];
  case PUGL_CURSOR_LEFT_RIGHT:
    return [NSCursor resizeLeftRightCursor];
  case PUGL_CURSOR_UP_DOWN:
    return [NSCursor resizeUpDownCursor];
  case PUGL_CURSOR_DIAGONAL:
    cursorSelector = @selector(_windowResizeNorthWestSouthEastCursor);
    break;
  case PUGL_CURSOR_ANTI_DIAGONAL:
    cursorSelector = @selector(_windowResizeNorthEastSouthWestCursor);
    break;
  }

  if (cursorSelector && [NSCursor respondsToSelector:cursorSelector])
  {
    id object = [NSCursor performSelector:cursorSelector];
    if ([object isKindOfClass:[NSCursor class]])
      return (NSCursor*)object;
  }

  return NULL;
}

PuglStatus
puglSetCursor(PuglView* view, PuglCursor cursor)
{
  PuglInternals* const impl = view->impl;
  NSCursor* const      cur  = puglGetNsCursor(cursor);
  if (!cur) {
    return PUGL_FAILURE;
  }

  impl->cursor = cur;

  if (impl->mouseTracked) {
    [cur set];
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglSetClipboard(PuglView* const   view,
                 const char* const type,
                 const void* const data,
                 const size_t      len)
{
  NSPasteboard* const pasteboard = [NSPasteboard generalPasteboard];
  const char* const   str        = (const char*)data;

  PuglStatus st = puglSetInternalClipboard(view, type, data, len);
  if (st) {
    return st;
  }

  NSString* nsString = [NSString stringWithUTF8String:str];
  if (nsString) {
    [pasteboard declareTypes:[NSArray arrayWithObjects:NSStringPboardType, nil]
                       owner:nil];

    [pasteboard setString:nsString forType:NSStringPboardType];

    return PUGL_SUCCESS;
  }

  return PUGL_UNKNOWN_ERROR;
}
