// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_PUGL_H
#define PUGL_PUGL_H

#include <stddef.h>
#include <stdint.h>

#ifndef __cplusplus
#  include <stdbool.h>
#endif

#ifndef PUGL_API
#  if defined(_WIN32) && !defined(PUGL_STATIC) && defined(PUGL_INTERNAL)
#    define PUGL_API __declspec(dllexport)
#  elif defined(_WIN32) && !defined(PUGL_STATIC)
#    define PUGL_API __declspec(dllimport)
#  elif defined(__GNUC__)
#    define PUGL_API __attribute__((visibility("default")))
#  else
#    define PUGL_API
#  endif
#endif

#ifndef PUGL_DISABLE_DEPRECATED
#  if defined(__clang__)
#    define PUGL_DEPRECATED_BY(rep) __attribute__((deprecated("", rep)))
#  elif defined(__GNUC__)
#    define PUGL_DEPRECATED_BY(rep) __attribute__((deprecated("Use " rep)))
#  else
#    define PUGL_DEPRECATED_BY(rep)
#  endif
#endif

#if defined(__GNUC__)
#  define PUGL_CONST_FUNC __attribute__((const))
#else
#  define PUGL_CONST_FUNC
#endif

#define PUGL_CONST_API \
  PUGL_API             \
  PUGL_CONST_FUNC

#define PUGL_BEGIN_DECLS
#define PUGL_END_DECLS

PUGL_BEGIN_DECLS

/**
   @defgroup pugl Pugl C API
   Pugl C API.
   @{
*/

/**
   A pixel coordinate within/of a view.

   This is relative to the top left corner of the view's parent, or to the top
   left corner of the view itself, depending on the context.

   There are platform-imposed limits on window positions.  For portability,
   applications should keep coordinates between -16000 and 16000.  Note that
   negative frame coordinates are possible, for example with multiple screens.
*/
typedef int16_t PuglCoord;

/**
   A pixel span (width or height) within/of a view.

   Due to platform limits, the span of a view in either dimension should be
   between 1 and 10000.
*/
typedef uint16_t PuglSpan;

/**
   A rectangle in a view or on the screen.

   This type is used to describe two things: the position and size of a view
   (for configuring), or a rectangle within a view (for exposing).

   The coordinate (0, 0) represents the top-left pixel of the parent window (or
   display if there isn't one), or the top-left pixel of the view,
   respectively.
*/
typedef struct {
  PuglCoord x;
  PuglCoord y;
  PuglSpan  width;
  PuglSpan  height;
} PuglRect;

/**
   @defgroup events Events

   All updates to the view happen via events, which are dispatched to the
   view's event function.  Most events map directly to one from the underlying
   window system, but some are constructed by Pugl itself so there is not
   necessarily a direct correspondence.

   @{
*/

/// Keyboard modifier flags
typedef enum {
  PUGL_MOD_SHIFT = 1u << 0u, ///< Shift key
  PUGL_MOD_CTRL  = 1u << 1u, ///< Control key
  PUGL_MOD_ALT   = 1u << 2u, ///< Alt/Option key
  PUGL_MOD_SUPER = 1u << 3u  ///< Mod4/Command/Windows key
} PuglMod;

/// Bitwise OR of #PuglMod values
typedef uint32_t PuglMods;

/**
   Keyboard key codepoints.

   All keys are identified by a Unicode code point in PuglKeyEvent::key.  This
   enumeration defines constants for special keys that do not have a standard
   code point, and some convenience constants for control characters.  Note
   that all keys are handled in the same way, this enumeration is just for
   convenience when writing hard-coded key bindings.

   Keys that do not have a standard code point use values in the Private Use
   Area in the Basic Multilingual Plane (`U+E000` to `U+F8FF`).  Applications
   must take care to not interpret these values beyond key detection, the
   mapping used here is arbitrary and specific to Pugl.
*/
typedef enum {
  // ASCII control codes
  PUGL_KEY_BACKSPACE = 0x08,
  PUGL_KEY_ESCAPE    = 0x1B,
  PUGL_KEY_DELETE    = 0x7F,

  // Unicode Private Use Area
  PUGL_KEY_F1 = 0xE000,
  PUGL_KEY_F2,
  PUGL_KEY_F3,
  PUGL_KEY_F4,
  PUGL_KEY_F5,
  PUGL_KEY_F6,
  PUGL_KEY_F7,
  PUGL_KEY_F8,
  PUGL_KEY_F9,
  PUGL_KEY_F10,
  PUGL_KEY_F11,
  PUGL_KEY_F12,
  PUGL_KEY_LEFT,
  PUGL_KEY_UP,
  PUGL_KEY_RIGHT,
  PUGL_KEY_DOWN,
  PUGL_KEY_PAGE_UP,
  PUGL_KEY_PAGE_DOWN,
  PUGL_KEY_HOME,
  PUGL_KEY_END,
  PUGL_KEY_INSERT,
  PUGL_KEY_SHIFT,
  PUGL_KEY_SHIFT_L = PUGL_KEY_SHIFT,
  PUGL_KEY_SHIFT_R,
  PUGL_KEY_CTRL,
  PUGL_KEY_CTRL_L = PUGL_KEY_CTRL,
  PUGL_KEY_CTRL_R,
  PUGL_KEY_ALT,
  PUGL_KEY_ALT_L = PUGL_KEY_ALT,
  PUGL_KEY_ALT_R,
  PUGL_KEY_SUPER,
  PUGL_KEY_SUPER_L = PUGL_KEY_SUPER,
  PUGL_KEY_SUPER_R,
  PUGL_KEY_MENU,
  PUGL_KEY_CAPS_LOCK,
  PUGL_KEY_SCROLL_LOCK,
  PUGL_KEY_NUM_LOCK,
  PUGL_KEY_PRINT_SCREEN,
  PUGL_KEY_PAUSE
} PuglKey;

/// The type of a PuglEvent
typedef enum {
  PUGL_NOTHING,        ///< No event
  PUGL_CREATE,         ///< View created, a #PuglCreateEvent
  PUGL_DESTROY,        ///< View destroyed, a #PuglDestroyEvent
  PUGL_CONFIGURE,      ///< View moved/resized, a #PuglConfigureEvent
  PUGL_MAP,            ///< View made visible, a #PuglMapEvent
  PUGL_UNMAP,          ///< View made invisible, a #PuglUnmapEvent
  PUGL_UPDATE,         ///< View ready to draw, a #PuglUpdateEvent
  PUGL_EXPOSE,         ///< View must be drawn, a #PuglExposeEvent
  PUGL_CLOSE,          ///< View will be closed, a #PuglCloseEvent
  PUGL_FOCUS_IN,       ///< Keyboard focus entered view, a #PuglFocusEvent
  PUGL_FOCUS_OUT,      ///< Keyboard focus left view, a #PuglFocusEvent
  PUGL_KEY_PRESS,      ///< Key pressed, a #PuglKeyEvent
  PUGL_KEY_RELEASE,    ///< Key released, a #PuglKeyEvent
  PUGL_TEXT,           ///< Character entered, a #PuglTextEvent
  PUGL_POINTER_IN,     ///< Pointer entered view, a #PuglCrossingEvent
  PUGL_POINTER_OUT,    ///< Pointer left view, a #PuglCrossingEvent
  PUGL_BUTTON_PRESS,   ///< Mouse button pressed, a #PuglButtonEvent
  PUGL_BUTTON_RELEASE, ///< Mouse button released, a #PuglButtonEvent
  PUGL_MOTION,         ///< Pointer moved, a #PuglMotionEvent
  PUGL_SCROLL,         ///< Scrolled, a #PuglScrollEvent
  PUGL_CLIENT,         ///< Custom client message, a #PuglClientEvent
  PUGL_TIMER,          ///< Timer triggered, a #PuglTimerEvent
  PUGL_LOOP_ENTER,     ///< Recursive loop entered, a #PuglLoopEnterEvent
  PUGL_LOOP_LEAVE,     ///< Recursive loop left, a #PuglLoopLeaveEvent
  PUGL_DATA_OFFER,     ///< Data offered from clipboard, a #PuglDataOfferEvent
  PUGL_DATA,           ///< Data available from clipboard, a #PuglDataEvent
} PuglEventType;

/// Common flags for all event types
typedef enum {
  PUGL_IS_SEND_EVENT = 1, ///< Event is synthetic
  PUGL_IS_HINT       = 2  ///< Event is a hint (not direct user input)
} PuglEventFlag;

/// Bitwise OR of #PuglEventFlag values
typedef uint32_t PuglEventFlags;

/// Reason for a PuglCrossingEvent
typedef enum {
  PUGL_CROSSING_NORMAL, ///< Crossing due to pointer motion
  PUGL_CROSSING_GRAB,   ///< Crossing due to a grab
  PUGL_CROSSING_UNGRAB  ///< Crossing due to a grab release
} PuglCrossingMode;

/**
   Scroll direction.

   Describes the direction of a #PuglScrollEvent along with whether the scroll
   is a "smooth" scroll.  The discrete directions are for devices like mouse
   wheels with constrained axes, while a smooth scroll is for those with
   arbitrary scroll direction freedom, like some touchpads.
*/
typedef enum {
  PUGL_SCROLL_UP,    ///< Scroll up
  PUGL_SCROLL_DOWN,  ///< Scroll down
  PUGL_SCROLL_LEFT,  ///< Scroll left
  PUGL_SCROLL_RIGHT, ///< Scroll right
  PUGL_SCROLL_SMOOTH ///< Smooth scroll in any direction
} PuglScrollDirection;

/// Common header for all event structs
typedef struct {
  PuglEventType  type;  ///< Event type
  PuglEventFlags flags; ///< Bitwise OR of #PuglEventFlag values
} PuglAnyEvent;

/**
   View create event.

   This event is sent when a view is realized before it is first displayed,
   with the graphics context entered.  This is typically used for setting up
   the graphics system, for example by loading OpenGL extensions.

   This event type has no extra fields.
*/
typedef PuglAnyEvent PuglCreateEvent;

/**
   View destroy event.

   This event is the counterpart to #PuglCreateEvent, and it is sent when the
   view is being destroyed.  This is typically used for tearing down the
   graphics system, or otherwise freeing any resources allocated when the
   create event was handled.

   This is the last event sent to any view, and immediately after it is
   processed, the view is destroyed and may no longer be used.

   This event type has no extra fields.
*/
typedef PuglAnyEvent PuglDestroyEvent;

/**
   View resize or move event.

   A configure event is sent whenever the view is resized or moved.  When a
   configure event is received, the graphics context is active but not set up
   for drawing.  For example, it is valid to adjust the OpenGL viewport or
   otherwise configure the context, but not to draw anything.
*/
typedef struct {
  PuglEventType  type;   ///< #PUGL_CONFIGURE
  PuglEventFlags flags;  ///< Bitwise OR of #PuglEventFlag values
  PuglCoord      x;      ///< Parent-relative X coordinate of view
  PuglCoord      y;      ///< Parent-relative Y coordinate of view
  PuglSpan       width;  ///< Width of view
  PuglSpan       height; ///< Height of view
} PuglConfigureEvent;

/**
   View show event.

   This event is sent when a view is mapped to the screen and made visible.

   This event type has no extra fields.
*/
typedef PuglAnyEvent PuglMapEvent;

/**
   View hide event.

   This event is sent when a view is unmapped from the screen and made
   invisible.

   This event type has no extra fields.
*/
typedef PuglAnyEvent PuglUnmapEvent;

/**
   View update event.

   This event is sent to every view near the end of a main loop iteration when
   any pending exposures are about to be redrawn.  It is typically used to mark
   regions to expose with puglPostRedisplay() or puglPostRedisplayRect().  For
   example, to continuously animate, a view calls puglPostRedisplay() when an
   update event is received, and it will then shortly receive an expose event.
*/
typedef PuglAnyEvent PuglUpdateEvent;

/**
   Expose event for when a region must be redrawn.

   When an expose event is received, the graphics context is active, and the
   view must draw the entire specified region.  The contents of the region are
   undefined, there is no preservation of anything drawn previously.
*/
typedef struct {
  PuglEventType  type;   ///< #PUGL_EXPOSE
  PuglEventFlags flags;  ///< Bitwise OR of #PuglEventFlag values
  PuglCoord      x;      ///< View-relative top-left X coordinate of region
  PuglCoord      y;      ///< View-relative top-left Y coordinate of region
  PuglSpan       width;  ///< Width of exposed region
  PuglSpan       height; ///< Height of exposed region
} PuglExposeEvent;

/**
   View close event.

   This event is sent when the view is to be closed, for example when the user
   clicks the close button.

   This event type has no extra fields.
*/
typedef PuglAnyEvent PuglCloseEvent;

/**
   Keyboard focus event.

   This event is sent whenever the view gains or loses the keyboard focus.  The
   view with the keyboard focus will receive any key press or release events.
*/
typedef struct {
  PuglEventType    type;  ///< #PUGL_FOCUS_IN or #PUGL_FOCUS_OUT
  PuglEventFlags   flags; ///< Bitwise OR of #PuglEventFlag values
  PuglCrossingMode mode;  ///< Reason for focus change
} PuglFocusEvent;

/**
   Key press or release event.

   This event represents low-level key presses and releases.  This can be used
   for "direct" keyboard handing like key bindings, but must not be interpreted
   as text input.

   Keys are represented portably as Unicode code points, using the "natural"
   code point for the key where possible (see #PuglKey for details).  The `key`
   field is the code for the pressed key, without any modifiers applied.  For
   example, a press or release of the 'A' key will have `key` 97 ('a')
   regardless of whether shift or control are being held.

   Alternatively, the raw `keycode` can be used to work directly with physical
   keys, but note that this value is not portable and differs between platforms
   and hardware.
*/
typedef struct {
  PuglEventType  type;    ///< #PUGL_KEY_PRESS or #PUGL_KEY_RELEASE
  PuglEventFlags flags;   ///< Bitwise OR of #PuglEventFlag values
  double         time;    ///< Time in seconds
  double         x;       ///< View-relative X coordinate
  double         y;       ///< View-relative Y coordinate
  double         xRoot;   ///< Root-relative X coordinate
  double         yRoot;   ///< Root-relative Y coordinate
  PuglMods       state;   ///< Bitwise OR of #PuglMod flags
  uint32_t       keycode; ///< Raw key code
  uint32_t       key;     ///< Unshifted Unicode character code, or 0
} PuglKeyEvent;

/**
   Character input event.

   This event represents text input, usually as the result of a key press.  The
   text is given both as a Unicode character code and a UTF-8 string.

   Note that this event is generated by the platform's input system, so there
   is not necessarily a direct correspondence between text events and physical
   key presses.  For example, with some input methods a sequence of several key
   presses will generate a single character.
*/
typedef struct {
  PuglEventType  type;      ///< #PUGL_TEXT
  PuglEventFlags flags;     ///< Bitwise OR of #PuglEventFlag values
  double         time;      ///< Time in seconds
  double         x;         ///< View-relative X coordinate
  double         y;         ///< View-relative Y coordinate
  double         xRoot;     ///< Root-relative X coordinate
  double         yRoot;     ///< Root-relative Y coordinate
  PuglMods       state;     ///< Bitwise OR of #PuglMod flags
  uint32_t       keycode;   ///< Raw key code
  uint32_t       character; ///< Unicode character code
  char           string[8]; ///< UTF-8 string
} PuglTextEvent;

/**
   Pointer enter or leave event.

   This event is sent when the pointer enters or leaves the view.  This can
   happen for several reasons (not just the user dragging the pointer over the
   window edge), as described by the `mode` field.
*/
typedef struct {
  PuglEventType    type;  ///< #PUGL_POINTER_IN or #PUGL_POINTER_OUT
  PuglEventFlags   flags; ///< Bitwise OR of #PuglEventFlag values
  double           time;  ///< Time in seconds
  double           x;     ///< View-relative X coordinate
  double           y;     ///< View-relative Y coordinate
  double           xRoot; ///< Root-relative X coordinate
  double           yRoot; ///< Root-relative Y coordinate
  PuglMods         state; ///< Bitwise OR of #PuglMod flags
  PuglCrossingMode mode;  ///< Reason for crossing
} PuglCrossingEvent;

/**
   Button press or release event.

   Button numbers start from 0, and are ordered: primary, secondary, middle.
   So, on a typical right-handed mouse, the button numbers are:

   Left: 0
   Right: 1
   Middle (often a wheel): 2

   Higher button numbers are reported in the same order they are represented on
   the system.  There is no universal standard here, but buttons 3 and 4 are
   typically a pair of buttons or a rocker, which are usually bound to "back"
   and "forward" operations.

   Note that these numbers may differ from those used on the underlying
   platform, since they are manipulated to provide a consistent portable API.
*/
typedef struct {
  PuglEventType  type;   ///< #PUGL_BUTTON_PRESS or #PUGL_BUTTON_RELEASE
  PuglEventFlags flags;  ///< Bitwise OR of #PuglEventFlag values
  double         time;   ///< Time in seconds
  double         x;      ///< View-relative X coordinate
  double         y;      ///< View-relative Y coordinate
  double         xRoot;  ///< Root-relative X coordinate
  double         yRoot;  ///< Root-relative Y coordinate
  PuglMods       state;  ///< Bitwise OR of #PuglMod flags
  uint32_t       button; ///< Button number starting from 0
} PuglButtonEvent;

/**
   Pointer motion event.
*/
typedef struct {
  PuglEventType  type;  ///< #PUGL_MOTION
  PuglEventFlags flags; ///< Bitwise OR of #PuglEventFlag values
  double         time;  ///< Time in seconds
  double         x;     ///< View-relative X coordinate
  double         y;     ///< View-relative Y coordinate
  double         xRoot; ///< Root-relative X coordinate
  double         yRoot; ///< Root-relative Y coordinate
  PuglMods       state; ///< Bitwise OR of #PuglMod flags
} PuglMotionEvent;

/**
   Scroll event.

   The scroll distance is expressed in "lines", an arbitrary unit that
   corresponds to a single tick of a detented mouse wheel.  For example, `dy` =
   1.0 scrolls 1 line up.  Some systems and devices support finer resolution
   and/or higher values for fast scrolls, so programs should handle any value
   gracefully.
*/
typedef struct {
  PuglEventType       type;      ///< #PUGL_SCROLL
  PuglEventFlags      flags;     ///< Bitwise OR of #PuglEventFlag values
  double              time;      ///< Time in seconds
  double              x;         ///< View-relative X coordinate
  double              y;         ///< View-relative Y coordinate
  double              xRoot;     ///< Root-relative X coordinate
  double              yRoot;     ///< Root-relative Y coordinate
  PuglMods            state;     ///< Bitwise OR of #PuglMod flags
  PuglScrollDirection direction; ///< Scroll direction
  double              dx;        ///< Scroll X distance in lines
  double              dy;        ///< Scroll Y distance in lines
} PuglScrollEvent;

/**
   Custom client message event.

   This can be used to send a custom message to a view, which is delivered via
   the window system and processed in the event loop as usual.  Among other
   things, this makes it possible to wake up the event loop for any reason.
*/
typedef struct {
  PuglEventType  type;  ///< #PUGL_CLIENT
  PuglEventFlags flags; ///< Bitwise OR of #PuglEventFlag values
  uintptr_t      data1; ///< Client-specific data
  uintptr_t      data2; ///< Client-specific data
} PuglClientEvent;

/**
   Timer event.

   This event is sent at the regular interval specified in the call to
   puglStartTimer() that activated it.

   The `id` is the application-specific ID given to puglStartTimer() which
   distinguishes this timer from others.  It should always be checked in the
   event handler, even in applications that register only one timer.
*/
typedef struct {
  PuglEventType  type;  ///< #PUGL_TIMER
  PuglEventFlags flags; ///< Bitwise OR of #PuglEventFlag values
  uintptr_t      id;    ///< Timer ID
} PuglTimerEvent;

/**
   Clipboard data offer event.

   This event is sent when a clipboard has data present, possibly with several
   datatypes.  While handling this event, the types can be investigated with
   puglGetClipboardType() to decide whether to accept the offer with
   puglAcceptOffer().
*/
typedef struct {
  PuglEventType  type;  ///< #PUGL_DATA_OFFER
  PuglEventFlags flags; ///< Bitwise OR of #PuglEventFlag values
  double         time;  ///< Time in seconds
} PuglDataOfferEvent;

/**
   Clipboard data event.

   This event is sent after accepting a data offer when the data has been
   retrieved and converted.  While handling this event, the data can be
   accessed with puglGetClipboard().
*/
typedef struct {
  PuglEventType  type;      ///< #PUGL_DATA
  PuglEventFlags flags;     ///< Bitwise OR of #PuglEventFlag values
  double         time;      ///< Time in seconds
  uint32_t       typeIndex; ///< Index of datatype
} PuglDataEvent;

/**
   Recursive loop enter event.

   This event is sent when the window system enters a recursive loop.  The main
   loop will be stalled and no expose events will be received while in the
   recursive loop.  To give the application full control, Pugl does not do any
   special handling of this situation, but this event can be used to install a
   timer to perform continuous actions (such as drawing) on platforms that do
   this.

   - MacOS: A recursive loop is entered while the window is being live resized.

   - Windows: A recursive loop is entered while the window is being live
     resized or the menu is shown.

   - X11: A recursive loop is never entered and the event loop runs as usual
     while the view is being resized.

   This event type has no extra fields.
*/
typedef PuglAnyEvent PuglLoopEnterEvent;

/**
   Recursive loop leave event.

   This event is sent after a loop enter event when the recursive loop is
   finished and normal iteration will continue.

   This event type has no extra fields.
*/
typedef PuglAnyEvent PuglLoopLeaveEvent;

/**
   View event.

   This is a union of all event types.  The type must be checked to determine
   which fields are safe to access.  A pointer to PuglEvent can either be cast
   to the appropriate type, or the union members used.

   The graphics system may only be accessed when handling certain events.  The
   graphics context is active for #PUGL_CREATE, #PUGL_DESTROY, #PUGL_CONFIGURE,
   and #PUGL_EXPOSE, but only enabled for drawing for #PUGL_EXPOSE.
*/
typedef union {
  PuglAnyEvent       any;       ///< Valid for all event types
  PuglEventType      type;      ///< Event type
  PuglButtonEvent    button;    ///< #PUGL_BUTTON_PRESS, #PUGL_BUTTON_RELEASE
  PuglConfigureEvent configure; ///< #PUGL_CONFIGURE
  PuglExposeEvent    expose;    ///< #PUGL_EXPOSE
  PuglKeyEvent       key;       ///< #PUGL_KEY_PRESS, #PUGL_KEY_RELEASE
  PuglTextEvent      text;      ///< #PUGL_TEXT
  PuglCrossingEvent  crossing;  ///< #PUGL_POINTER_IN, #PUGL_POINTER_OUT
  PuglMotionEvent    motion;    ///< #PUGL_MOTION
  PuglScrollEvent    scroll;    ///< #PUGL_SCROLL
  PuglFocusEvent     focus;     ///< #PUGL_FOCUS_IN, #PUGL_FOCUS_OUT
  PuglClientEvent    client;    ///< #PUGL_CLIENT
  PuglTimerEvent     timer;     ///< #PUGL_TIMER
  PuglDataOfferEvent offer;     ///< #PUGL_DATA_OFFER
  PuglDataEvent      data;      ///< #PUGL_DATA
} PuglEvent;

/**
   @}
   @defgroup status Status

   Most functions return a status code which can be used to check for errors.

   @{
*/

/// Return status code
typedef enum {
  PUGL_SUCCESS,               ///< Success
  PUGL_FAILURE,               ///< Non-fatal failure
  PUGL_UNKNOWN_ERROR,         ///< Unknown system error
  PUGL_BAD_BACKEND,           ///< Invalid or missing backend
  PUGL_BAD_CONFIGURATION,     ///< Invalid view configuration
  PUGL_BAD_PARAMETER,         ///< Invalid parameter
  PUGL_BACKEND_FAILED,        ///< Backend initialization failed
  PUGL_REGISTRATION_FAILED,   ///< Class registration failed
  PUGL_REALIZE_FAILED,        ///< System view realization failed
  PUGL_SET_FORMAT_FAILED,     ///< Failed to set pixel format
  PUGL_CREATE_CONTEXT_FAILED, ///< Failed to create drawing context
  PUGL_UNSUPPORTED,           ///< Unsupported operation
  PUGL_NO_MEMORY,             ///< Failed to allocate memory
} PuglStatus;

/// Return a string describing a status code
PUGL_CONST_API
const char*
puglStrerror(PuglStatus status);

/**
   @}
   @defgroup world World

   The top-level context of a Pugl application or plugin.

   The world contains all library-wide state.  There is no static data in Pugl,
   so it is safe to use multiple worlds in a single process.  This is to
   facilitate plugins or other situations where it is not possible to share a
   world, but a single world should be shared for all views where possible.

   @{
*/

/**
   The "world" of application state.

   The world represents everything that is not associated with a particular
   view.  Several worlds can be created in a single process, but code using
   different worlds must be isolated so they are never mixed.  Views are
   strongly associated with the world they were created in.
*/
typedef struct PuglWorldImpl PuglWorld;

/// Handle for the world's opaque user data
typedef void* PuglWorldHandle;

/// The type of a World
typedef enum {
  PUGL_PROGRAM, ///< Top-level application
  PUGL_MODULE   ///< Plugin or module within a larger application
} PuglWorldType;

/// World flags
typedef enum {
  /**
     Set up support for threads if necessary.

     X11: Calls XInitThreads() which is required for some drivers.
  */
  PUGL_WORLD_THREADS = 1u << 0u
} PuglWorldFlag;

/// Bitwise OR of #PuglWorldFlag values
typedef uint32_t PuglWorldFlags;

/**
   Create a new world.

   @param type The type, which dictates what this world is responsible for.
   @param flags Flags to control world features.
   @return A new world, which must be later freed with puglFreeWorld().
*/
PUGL_API
PuglWorld*
puglNewWorld(PuglWorldType type, PuglWorldFlags flags);

/// Free a world allocated with puglNewWorld()
PUGL_API
void
puglFreeWorld(PuglWorld* world);

/**
   Set the user data for the world.

   This is usually a pointer to a struct that contains all the state which must
   be accessed by several views.

   The handle is opaque to Pugl and is not interpreted in any way.
*/
PUGL_API
void
puglSetWorldHandle(PuglWorld* world, PuglWorldHandle handle);

/// Get the user data for the world
PUGL_API
PuglWorldHandle
puglGetWorldHandle(PuglWorld* world);

/**
   Return a pointer to the native handle of the world.

   X11: Returns a pointer to the `Display`.

   MacOS: Returns a pointer to the `NSApplication`.

   Windows: Returns the `HMODULE` of the calling process.
*/
PUGL_API
void*
puglGetNativeWorld(PuglWorld* world);

/**
   Set the class name of the application.

   This is a stable identifier for the application, used as the window
   class/instance name on X11 and Windows.  It is not displayed to the user,
   but can be used in scripts and by window managers, so it should be the same
   for every instance of the application, but different from other
   applications.
*/
PUGL_API
PuglStatus
puglSetClassName(PuglWorld* world, const char* name);

/// Get the class name of the application, or null
PUGL_API
const char*
puglGetClassName(const PuglWorld* world);

/**
   Return the time in seconds.

   This is a monotonically increasing clock with high resolution.  The returned
   time is only useful to compare against other times returned by this
   function, its absolute value has no meaning.
*/
PUGL_API
double
puglGetTime(const PuglWorld* world);

/**
   Update by processing events from the window system.

   This function is a single iteration of the main loop, and should be called
   repeatedly to update all views.

   If `timeout` is zero, then this function will not block.  Plugins should
   always use a timeout of zero to avoid blocking the host.

   If a positive `timeout` is given, then events will be processed for that
   amount of time, starting from when this function was called.

   If a negative `timeout` is given, this function will block indefinitely
   until an event occurs.

   For continuously animating programs, a timeout that is a reasonable fraction
   of the ideal frame period should be used, to minimize input latency by
   ensuring that as many input events are consumed as possible before drawing.

   @return #PUGL_SUCCESS if events are read, #PUGL_FAILURE if no events are
   read, or an error.
*/
PUGL_API
PuglStatus
puglUpdate(PuglWorld* world, double timeout);

/**
   @}
   @defgroup view View

   A drawable region that receives events.

   A view can be thought of as a window, but does not necessarily correspond to
   a top-level window in a desktop environment.  For example, a view can be
   embedded in some other window, or represent an embedded system where there
   is no concept of multiple windows at all.

   @{
*/

/// A drawable region that receives events
typedef struct PuglViewImpl PuglView;

/**
   A graphics backend.

   The backend dictates how graphics are set up for a view, and how drawing is
   performed.  A backend must be set by calling puglSetBackend() before
   realising a view.

   If you are using a local copy of Pugl, it is possible to implement a custom
   backend.  See the definition of `PuglBackendImpl` in the source code for
   details.
*/
typedef struct PuglBackendImpl PuglBackend;

/**
   A native view handle.

   X11: This is a `Window`.

   MacOS: This is a pointer to an `NSView*`.

   Windows: This is a `HWND`.
*/
typedef uintptr_t PuglNativeView;

/// Handle for a view's opaque user data
typedef void* PuglHandle;

/// A hint for configuring a view
typedef enum {
  PUGL_USE_COMPAT_PROFILE,    ///< Use compatible (not core) OpenGL profile
  PUGL_USE_DEBUG_CONTEXT,     ///< True to use a debug OpenGL context
  PUGL_CONTEXT_VERSION_MAJOR, ///< OpenGL context major version
  PUGL_CONTEXT_VERSION_MINOR, ///< OpenGL context minor version
  PUGL_RED_BITS,              ///< Number of bits for red channel
  PUGL_GREEN_BITS,            ///< Number of bits for green channel
  PUGL_BLUE_BITS,             ///< Number of bits for blue channel
  PUGL_ALPHA_BITS,            ///< Number of bits for alpha channel
  PUGL_DEPTH_BITS,            ///< Number of bits for depth buffer
  PUGL_STENCIL_BITS,          ///< Number of bits for stencil buffer
  PUGL_SAMPLES,               ///< Number of samples per pixel (AA)
  PUGL_DOUBLE_BUFFER,         ///< True if double buffering should be used
  PUGL_SWAP_INTERVAL,         ///< Number of frames between buffer swaps
  PUGL_RESIZABLE,             ///< True if view should be resizable
  PUGL_IGNORE_KEY_REPEAT,     ///< True if key repeat events are ignored
  PUGL_REFRESH_RATE,          ///< Refresh rate in Hz
} PuglViewHint;

/// The number of #PuglViewHint values
#define PUGL_NUM_VIEW_HINTS ((unsigned)PUGL_REFRESH_RATE + 1u)

/// A special view hint value
typedef enum {
  PUGL_DONT_CARE = -1, ///< Use best available value
  PUGL_FALSE     = 0,  ///< Explicitly false
  PUGL_TRUE      = 1   ///< Explicitly true
} PuglViewHintValue;

/**
   A hint for configuring/constraining the size of a view.

   The system will attempt to make the view's window adhere to these, but they
   are suggestions, not hard constraints.  Applications should handle any view
   size gracefully.
*/
typedef enum {
  PUGL_DEFAULT_SIZE, ///< Default size
  PUGL_MIN_SIZE,     ///< Minimum size
  PUGL_MAX_SIZE,     ///< Maximum size

  /**
     Fixed aspect ratio.

     If set, the view's size should be constrained to this aspect ratio.
     Mutually exclusive with #PUGL_MIN_ASPECT and #PUGL_MAX_ASPECT.
  */
  PUGL_FIXED_ASPECT,

  /**
     Minimum aspect ratio.

     If set, the view's size should be constrained to an aspect ratio no lower
     than this.  Mutually exclusive with #PUGL_FIXED_ASPECT.
  */
  PUGL_MIN_ASPECT,

  /**
     Maximum aspect ratio.

     If set, the view's size should be constrained to an aspect ratio no higher
     than this.  Mutually exclusive with #PUGL_FIXED_ASPECT.
  */
  PUGL_MAX_ASPECT
} PuglSizeHint;

/// The number of #PuglSizeHint values
#define PUGL_NUM_SIZE_HINTS ((unsigned)PUGL_MAX_ASPECT + 1u)

/// A function called when an event occurs
typedef PuglStatus (*PuglEventFunc)(PuglView* view, const PuglEvent* event);

/**
   @defgroup setup Setup
   Functions for creating and destroying a view.
   @{
*/

/**
   Create a new view.

   A newly created view does not correspond to a real system view or window.
   It must first be configured, then the system view can be created with
   puglRealize().
*/
PUGL_API
PuglView*
puglNewView(PuglWorld* world);

/// Free a view created with puglNewView()
PUGL_API
void
puglFreeView(PuglView* view);

/// Return the world that `view` is a part of
PUGL_API
PuglWorld*
puglGetWorld(PuglView* view);

/**
   Set the user data for a view.

   This is usually a pointer to a struct that contains all the state which must
   be accessed by a view.  Everything needed to process events should be stored
   here, not in static variables.

   The handle is opaque to Pugl and is not interpreted in any way.
*/
PUGL_API
void
puglSetHandle(PuglView* view, PuglHandle handle);

/// Get the user data for a view
PUGL_API
PuglHandle
puglGetHandle(PuglView* view);

/**
   Set the graphics backend to use for a view.

   This must be called once to set the graphics backend before calling
   puglRealize().

   Pugl includes the following backends:

   - puglCairoBackend()
   - puglGlBackend()
   - puglVulkanBackend()

   Note that backends are modular and not compiled into the main Pugl library
   to avoid unnecessary dependencies.  To use a particular backend,
   applications must link against the appropriate backend library, or be sure
   to compile in the appropriate code if using a local copy of Pugl.
*/
PUGL_API
PuglStatus
puglSetBackend(PuglView* view, const PuglBackend* backend);

/// Return the graphics backend used by a view
const PuglBackend*
puglGetBackend(const PuglView* view);

/// Set the function to call when an event occurs
PUGL_API
PuglStatus
puglSetEventFunc(PuglView* view, PuglEventFunc eventFunc);

/**
   Set a hint to configure view properties.

   This only has an effect when called before puglRealize().
*/
PUGL_API
PuglStatus
puglSetViewHint(PuglView* view, PuglViewHint hint, int value);

/**
   Get the value for a view hint.

   If the view has been realized, this can be used to get the actual value of a
   hint which was initially set to PUGL_DONT_CARE, or has been adjusted from
   the suggested value.
*/
PUGL_API
int
puglGetViewHint(const PuglView* view, PuglViewHint hint);

/**
   Return the scale factor of the view.

   This factor describe how large UI elements (especially text) should be
   compared to "normal".  For example, 2.0 means the UI should be drawn twice
   as large.

   "Normal" is loosely defined, but means a good size on a "standard DPI"
   display (around 96 DPI).  In other words, the scale 1.0 should have text
   that is reasonably sized on a 96 DPI display, and the scale 2.0 should have
   text twice that large.
*/
PUGL_API
double
puglGetScaleFactor(const PuglView* view);

/**
   @}
   @defgroup frame Frame
   Functions for working with the position and size of a view.
   @{
*/

/**
   Get the current position and size of the view.

   The position is in screen coordinates with an upper left origin.
*/
PUGL_API
PuglRect
puglGetFrame(const PuglView* view);

/**
   Set the current position and size of the view.

   The position is in screen coordinates with an upper left origin.

   @return #PUGL_UNKNOWN_ERROR on failure, in which case the view frame is
   unchanged.
*/
PUGL_API
PuglStatus
puglSetFrame(PuglView* view, PuglRect frame);

/**
   Set the current position of the view.

   @return #PUGL_UNKNOWN_ERROR on failure, in which case the view frame is
   unchanged.
*/
PUGL_API
PuglStatus
puglSetPosition(PuglView* view, int x, int y);

/**
   Set the current size of the view.

   @return #PUGL_UNKNOWN_ERROR on failure, in which case the view frame is
   unchanged.
*/
PUGL_API
PuglStatus
puglSetSize(PuglView* view, unsigned width, unsigned height);

/**
   Set a size hint for the view.

   This can be used to set the default, minimum, and maximum size of a view,
   as well as the supported range of aspect ratios.

   This should be called before puglRealize() so the initial window for the
   view can be configured correctly.

   @return #PUGL_UNKNOWN_ERROR on failure, but always succeeds if the view is
   not yet realized.
*/
PUGL_API
PuglStatus
puglSetSizeHint(PuglView*    view,
                PuglSizeHint hint,
                PuglSpan     width,
                PuglSpan     height);

/**
   @}
   @defgroup window Window
   Functions to control the top-level window of a view.
   @{
*/

/**
   Set the title of the window.

   This only makes sense for non-embedded views that will have a corresponding
   top-level window, and sets the title, typically displayed in the title bar
   or in window switchers.
*/
PUGL_API
PuglStatus
puglSetWindowTitle(PuglView* view, const char* title);

/// Return the title of the window, or null
PUGL_API
const char*
puglGetWindowTitle(const PuglView* view);

/**
   Set the parent window for embedding a view in an existing window.

   This must be called before puglRealize(), reparenting is not supported.
*/
PUGL_API
PuglStatus
puglSetParentWindow(PuglView* view, PuglNativeView parent);

/// Return the parent window this view is embedded in, or null
PUGL_API
PuglNativeView
puglGetParentWindow(const PuglView* view);

/**
   Set the transient parent of the window.

   Set this for transient children like dialogs, to have them properly
   associated with their parent window.  This should be called before
   puglRealize().

   A view can either have a parent (for embedding) or a transient parent (for
   top-level windows like dialogs), but not both.
*/
PUGL_API
PuglStatus
puglSetTransientParent(PuglView* view, PuglNativeView parent);

/**
   Return the transient parent of the window.

   @return The native handle to the window this view is a transient child of,
   or null.
*/
PUGL_API
PuglNativeView
puglGetTransientParent(const PuglView* view);

/**
   Realize a view by creating a corresponding system view or window.

   After this call, the (initially invisible) underlying system view exists and
   can be accessed with puglGetNativeView().  There is currently no
   corresponding unrealize function, the system view will be destroyed along
   with the view when puglFreeView() is called.

   The view should be fully configured using the above functions before this is
   called.  This function may only be called once per view.
*/
PUGL_API
PuglStatus
puglRealize(PuglView* view);

/**
   Show the view.

   If the view has not yet been realized, the first call to this function will
   do so automatically.

   If the view is currently hidden, it will be shown and possibly raised to the
   top depending on the platform.
*/
PUGL_API
PuglStatus
puglShow(PuglView* view);

/// Hide the current window
PUGL_API
PuglStatus
puglHide(PuglView* view);

/// Return true iff the view is currently visible
PUGL_API
bool
puglGetVisible(const PuglView* view);

/// Return the native window handle
PUGL_API
PuglNativeView
puglGetNativeView(PuglView* view);

/**
   @}
   @defgroup graphics Graphics
   Functions for working with the graphics context and scheduling redisplays.
   @{
*/

/**
   Get the graphics context.

   This is a backend-specific context used for drawing if the backend graphics
   API requires one.  It is only available during an expose.

   Cairo: Returns a pointer to a
   [cairo_t](http://www.cairographics.org/manual/cairo-cairo-t.html).

   All other backends: returns null.
*/
PUGL_API
void*
puglGetContext(PuglView* view);

/**
   Request a redisplay for the entire view.

   This will cause an expose event to be dispatched later.  If called from
   within the event handler, the expose should arrive at the end of the current
   event loop iteration, though this is not strictly guaranteed on all
   platforms.  If called elsewhere, an expose will be enqueued to be processed
   in the next event loop iteration.
*/
PUGL_API
PuglStatus
puglPostRedisplay(PuglView* view);

/**
   Request a redisplay of the given rectangle within the view.

   This has the same semantics as puglPostRedisplay(), but allows giving a
   precise region for redrawing only a portion of the view.
*/
PUGL_API
PuglStatus
puglPostRedisplayRect(PuglView* view, PuglRect rect);

/**
   @}
   @defgroup interaction Interaction
   Functions for interacting with the user and window system.
   @{
*/

/**
   A mouse cursor type.

   This is a portable subset of mouse cursors that exist on X11, MacOS, and
   Windows.
*/
typedef enum {
  PUGL_CURSOR_ARROW,         ///< Default pointing arrow
  PUGL_CURSOR_CARET,         ///< Caret (I-Beam) for text entry
  PUGL_CURSOR_CROSSHAIR,     ///< Cross-hair
  PUGL_CURSOR_HAND,          ///< Hand with a pointing finger
  PUGL_CURSOR_NO,            ///< Operation not allowed
  PUGL_CURSOR_LEFT_RIGHT,    ///< Left/right arrow for horizontal resize
  PUGL_CURSOR_UP_DOWN,       ///< Up/down arrow for vertical resize
  PUGL_CURSOR_DIAGONAL,      ///< Top-left to bottom-right arrow for diagonal resize
  PUGL_CURSOR_ANTI_DIAGONAL, ///< Bottom-left to top-right arrow for diagonal resize
} PuglCursor;

/// The number of #PuglCursor values
#define PUGL_NUM_CURSORS ((unsigned)PUGL_CURSOR_ANTI_DIAGONAL + 1u)

/**
   Grab the keyboard input focus.

   Note that this will fail if the view is not mapped and so should not, for
   example, be called immediately after puglShow().

   @return #PUGL_SUCCESS if the focus was successfully grabbed, or an error.
*/
PUGL_API
PuglStatus
puglGrabFocus(PuglView* view);

/// Return whether `view` has the keyboard input focus
PUGL_API
bool
puglHasFocus(const PuglView* view);

/**
   Request data from the general copy/paste clipboard.

   A #PUGL_DATA_OFFER event will be sent if data is available.
*/
PUGL_API
PuglStatus
puglPaste(PuglView* view);

/**
   Return the number of types available for the data in a clipboard.

   Returns zero if the clipboard is empty.
*/
PUGL_API
uint32_t
puglGetNumClipboardTypes(const PuglView* view);

/**
   Return the identifier of a type available in a clipboard.

   This is usually a MIME type, but may also be another platform-specific type
   identifier.  Applications must ignore any type they do not recognize.

   Returns null if `typeIndex` is out of bounds according to
   puglGetNumClipboardTypes().
*/
PUGL_API
const char*
puglGetClipboardType(const PuglView* view, uint32_t typeIndex);

/**
   Accept data offered from a clipboard.

   To accept data, this must be called while handling a #PUGL_DATA_OFFER event.
   Doing so will request the data from the source as the specified type.  When
   the data is available, a #PUGL_DATA event will be sent to the view which can
   then retrieve the data with puglGetClipboard().

   @param view The view.

   @param offer The data offer event.

   @param typeIndex The index of the type that the view will accept.  This is
   the `typeIndex` argument to the call of puglGetClipboardType() that returned
   the accepted type.
*/
PUGL_API
PuglStatus
puglAcceptOffer(PuglView*                 view,
                const PuglDataOfferEvent* offer,
                uint32_t                  typeIndex);

/**
   Set the clipboard contents.

   This sets the system clipboard contents, which can be retrieved with
   puglGetClipboard() or pasted into other applications.

   @param view The view.
   @param type The MIME type of the data, "text/plain" is assumed if `NULL`.
   @param data The data to copy to the clipboard.
   @param len The length of data in bytes (including terminator if necessary).
*/
PUGL_API
PuglStatus
puglSetClipboard(PuglView*   view,
                 const char* type,
                 const void* data,
                 size_t      len);

/**
   Get the clipboard contents.

   This gets the system clipboard contents, which may have been set with
   puglSetClipboard() or copied from another application.

   @param view The view.
   @param typeIndex Index of the data type to get the item as.
   @param[out] len Set to the length of the data in bytes.
   @return The clipboard contents, or null.
*/
PUGL_API
const void*
puglGetClipboard(PuglView* view, uint32_t typeIndex, size_t* len);

/**
   Set the mouse cursor.

   This changes the system cursor that is displayed when the pointer is inside
   the view.  May fail if setting the cursor is not supported on this system,
   for example if compiled on X11 without Xcursor support.

   @return #PUGL_BAD_PARAMETER if the given cursor is invalid,
   #PUGL_FAILURE if the cursor is known but loading it system fails.
*/
PUGL_API
PuglStatus
puglSetCursor(PuglView* view, PuglCursor cursor);

/**
   Request user attention.

   This hints to the system that the window or application requires attention
   from the user.  The exact effect depends on the platform, but is usually
   something like a flashing task bar entry or bouncing application icon.
*/
PUGL_API
PuglStatus
puglRequestAttention(PuglView* view);

/**
   Activate a repeating timer event.

   This starts a timer which will send a #PuglTimerEvent to `view` every
   `timeout` seconds.  This can be used to perform some action in a view at a
   regular interval with relatively low frequency.  Note that the frequency of
   timer events may be limited by how often puglUpdate() is called.

   If the given timer already exists, it is replaced.

   @param view The view to begin sending #PUGL_TIMER events to.

   @param id The identifier for this timer.  This is an application-specific ID
   that should be a low number, typically the value of a constant or `enum`
   that starts from 0.  There is a platform-specific limit to the number of
   supported timers, and overhead associated with each, so applications should
   create only a few timers and perform several tasks in one if necessary.

   @param timeout The period, in seconds, of this timer.  This is not
   guaranteed to have a resolution better than 10ms (the maximum timer
   resolution on Windows) and may be rounded up if it is too short.  On X11 and
   MacOS, a resolution of about 1ms can usually be relied on.

   @return #PUGL_FAILURE if timers are not supported by the system,
   #PUGL_UNKNOWN_ERROR if setting the timer failed.
*/
PUGL_API
PuglStatus
puglStartTimer(PuglView* view, uintptr_t id, double timeout);

/**
   Stop an active timer.

   @param view The view that the timer is set for.
   @param id The ID previously passed to puglStartTimer().

   @return #PUGL_FAILURE if timers are not supported by this system,
   #PUGL_UNKNOWN_ERROR if stopping the timer failed.
*/
PUGL_API
PuglStatus
puglStopTimer(PuglView* view, uintptr_t id);

/**
   Send an event to a view via the window system.

   If supported, the event will be delivered to the view via the event loop
   like other events.  Note that this function only works for certain event
   types.

   Currently, only #PUGL_CLIENT events are supported on all platforms.

   X11: A #PUGL_EXPOSE event can be sent, which is similar to calling
   puglPostRedisplayRect(), but will always send a message to the X server,
   even when called in an event handler.

   @return #PUGL_UNSUPPORTED if sending events of this type is not supported,
   #PUGL_UNKNOWN_ERROR if sending the event failed.
*/
PUGL_API
PuglStatus
puglSendEvent(PuglView* view, const PuglEvent* event);

/**
   @}
*/

#ifndef PUGL_DISABLE_DEPRECATED

/**
   @}
   @defgroup deprecated Deprecated API
   @{
*/

PUGL_DEPRECATED_BY("PuglCreateEvent")
typedef PuglCreateEvent PuglEventCreate;

PUGL_DEPRECATED_BY("PuglDestroyEvent")
typedef PuglDestroyEvent PuglEventDestroy;

PUGL_DEPRECATED_BY("PuglConfigureEvent")
typedef PuglConfigureEvent PuglEventConfigure;

PUGL_DEPRECATED_BY("PuglMapEvent")
typedef PuglMapEvent PuglEventMap;

PUGL_DEPRECATED_BY("PuglUnmapEvent")
typedef PuglUnmapEvent PuglEventUnmap;

PUGL_DEPRECATED_BY("PuglUpdateEvent")
typedef PuglUpdateEvent PuglEventUpdate;

PUGL_DEPRECATED_BY("PuglExposeEvent")
typedef PuglExposeEvent PuglEventExpose;

PUGL_DEPRECATED_BY("PuglCloseEvent")
typedef PuglCloseEvent PuglEventClose;

PUGL_DEPRECATED_BY("PuglFocusEvent")
typedef PuglFocusEvent PuglEventFocus;

PUGL_DEPRECATED_BY("PuglKeyEvent")
typedef PuglKeyEvent PuglEventKey;

PUGL_DEPRECATED_BY("PuglTextEvent")
typedef PuglTextEvent PuglEventText;

PUGL_DEPRECATED_BY("PuglCrossingEvent")
typedef PuglCrossingEvent PuglEventCrossing;

PUGL_DEPRECATED_BY("PuglButtonEvent")
typedef PuglButtonEvent PuglEventButton;

PUGL_DEPRECATED_BY("PuglMotionEvent")
typedef PuglMotionEvent PuglEventMotion;

PUGL_DEPRECATED_BY("PuglScrollEvent")
typedef PuglScrollEvent PuglEventScroll;

PUGL_DEPRECATED_BY("PuglClientEvent")
typedef PuglClientEvent PuglEventClient;

PUGL_DEPRECATED_BY("PuglTimerEvent")
typedef PuglTimerEvent PuglEventTimer;

PUGL_DEPRECATED_BY("PuglLoopEnterEvent")
typedef PuglLoopEnterEvent PuglEventLoopEnter;

PUGL_DEPRECATED_BY("PuglLoopLeaveEvent")
typedef PuglLoopLeaveEvent PuglEventLoopLeave;

/**
   A native window handle.

   X11: This is a `Window`.

   MacOS: This is a pointer to an `NSView*`.

   Windows: This is a `HWND`.
*/
PUGL_DEPRECATED_BY("PuglNativeView")
typedef uintptr_t PuglNativeWindow;

/**
   Create a Pugl application and view.

   To create a window, call the various puglInit* functions as necessary, then
   call puglRealize().

   @deprecated Use puglNewApp() and puglNewView().

   @param pargc Pointer to argument count (currently unused).
   @param argv  Arguments (currently unused).
   @return A newly created view.
*/
static inline PUGL_DEPRECATED_BY("puglNewView")
PuglView*
puglInit(const int* pargc, char** argv)
{
  (void)pargc;
  (void)argv;

  return puglNewView(puglNewWorld(PUGL_MODULE, 0));
}

/**
   Destroy an app and view created with `puglInit()`.

   @deprecated Use puglFreeApp() and puglFreeView().
*/
static inline PUGL_DEPRECATED_BY("puglFreeView")
void
puglDestroy(PuglView* view)
{
  PuglWorld* const world = puglGetWorld(view);

  puglFreeView(view);
  puglFreeWorld(world);
}

/**
   Set the window class name before creating a window.
*/
static inline PUGL_DEPRECATED_BY("puglSetClassName")
void
puglInitWindowClass(PuglView* view, const char* name)
{
  puglSetClassName(puglGetWorld(view), name);
}

/**
   Set the window size before creating a window.

   @deprecated Use puglSetFrame().
*/
static inline PUGL_DEPRECATED_BY("puglSetFrame")
void
puglInitWindowSize(PuglView* view, int width, int height)
{
  PuglRect frame = puglGetFrame(view);

  frame.width  = (PuglSpan)width;
  frame.height = (PuglSpan)height;

  puglSetFrame(view, frame);
}

/**
   Set the minimum window size before creating a window.
*/
static inline PUGL_DEPRECATED_BY("puglSetMinSize")
void
puglInitWindowMinSize(PuglView* view, int width, int height)
{
  puglSetSizeHint(view, PUGL_MIN_SIZE, (PuglSpan)width, (PuglSpan)height);
}

/**
   Set the window aspect ratio range before creating a window.

   The x and y values here represent a ratio of width to height.  To set a
   fixed aspect ratio, set the minimum and maximum values to the same ratio.

   Note that setting different minimum and maximum constraints does not
   currently work on MacOS (the minimum is used), so only setting a fixed
   aspect ratio works properly across all platforms.
*/
static inline PUGL_DEPRECATED_BY("puglSetAspectRatio")
void
puglInitWindowAspectRatio(PuglView* view,
                          int       minX,
                          int       minY,
                          int       maxX,
                          int       maxY)
{
  puglSetSizeHint(view, PUGL_MIN_ASPECT, (PuglSpan)minX, (PuglSpan)minY);
  puglSetSizeHint(view, PUGL_MAX_ASPECT, (PuglSpan)maxX, (PuglSpan)maxY);
}

/**
   Set transient parent before creating a window.

   On X11, parent must be a Window.
   On OSX, parent must be an NSView*.
*/
static inline PUGL_DEPRECATED_BY("puglSetTransientParent")
void
puglInitTransientFor(PuglView* view, uintptr_t parent)
{
  puglSetTransientParent(view, (PuglNativeWindow)parent);
}

/**
   Set transient parent before creating a window.

   @deprecated Use puglSetTransientParent().
*/
static inline PUGL_DEPRECATED_BY("puglSetTransientParent")
PuglStatus
puglSetTransientFor(PuglView* view, uintptr_t parent)
{
  return puglSetTransientParent(view, (PuglNativeWindow)parent);
}

/**
   Enable or disable resizing before creating a window.

   @deprecated Use puglSetViewHint() with #PUGL_RESIZABLE.
*/
static inline PUGL_DEPRECATED_BY("puglSetViewHint")
void
puglInitResizable(PuglView* view, bool resizable)
{
  puglSetViewHint(view, PUGL_RESIZABLE, resizable);
}

/**
   Get the current size of the view.

   @deprecated Use puglGetFrame().

*/
static inline PUGL_DEPRECATED_BY("puglGetFrame")
void
puglGetSize(PuglView* view, int* width, int* height)
{
  const PuglRect frame = puglGetFrame(view);

  *width  = (int)frame.width;
  *height = (int)frame.height;
}

/**
   Ignore synthetic repeated key events.

   @deprecated Use puglSetViewHint() with #PUGL_IGNORE_KEY_REPEAT.
*/
static inline PUGL_DEPRECATED_BY("puglSetViewHint")
void
puglIgnoreKeyRepeat(PuglView* view, bool ignore)
{
  puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, ignore);
}

/**
   Set a hint before creating a window.

   @deprecated Use puglSetWindowHint().
*/
static inline PUGL_DEPRECATED_BY("puglSetViewHint")
void
puglInitWindowHint(PuglView* view, PuglViewHint hint, int value)
{
  puglSetViewHint(view, hint, value);
}

/**
   Set the parent window before creating a window (for embedding).

   @deprecated Use puglSetWindowParent().
*/
static inline PUGL_DEPRECATED_BY("puglSetParentWindow")
void
puglInitWindowParent(PuglView* view, PuglNativeWindow parent)
{
  puglSetParentWindow(view, parent);
}

/**
   Set the graphics backend to use.

   @deprecated Use puglSetBackend().
*/
static inline PUGL_DEPRECATED_BY("puglSetBackend")
int
puglInitBackend(PuglView* view, const PuglBackend* backend)
{
  return (int)puglSetBackend(view, backend);
}

/**
   Realize a view by creating a corresponding system view or window.

   The view should be fully configured using the above functions before this is
   called.  This function may only be called once per view.

   @deprecated Use puglRealize(), or just show the view.
*/
static inline PUGL_DEPRECATED_BY("puglRealize")
PuglStatus
puglCreateWindow(PuglView* view, const char* title)
{
  puglSetWindowTitle(view, title);
  return puglRealize(view);
}

/**
   Block and wait for an event to be ready.

   This can be used in a loop to only process events via puglProcessEvents when
   necessary.  This function will block indefinitely if no events are
   available, so is not appropriate for use in programs that need to perform
   regular updates (e.g. animation).

   @deprecated Use puglPollEvents().
*/
PUGL_API
PUGL_DEPRECATED_BY("puglPollEvents")
PuglStatus
puglWaitForEvent(PuglView* view);

/**
   Process all pending window events.

   This handles input events as well as rendering, so it should be called
   regularly and rapidly enough to keep the UI responsive.  This function does
   not block if no events are pending.

   @deprecated Use puglDispatchEvents().
*/
PUGL_API
PUGL_DEPRECATED_BY("puglDispatchEvents")
PuglStatus
puglProcessEvents(PuglView* view);

/**
   Poll for events that are ready to be processed.

   This polls for events that are ready for any view in the world, potentially
   blocking depending on `timeout`.

   @param world The world to poll for events.

   @param timeout Maximum time to wait, in seconds.  If zero, the call returns
   immediately, if negative, the call blocks indefinitely.

   @return #PUGL_SUCCESS if events are read, #PUGL_FAILURE if not, or an error.

   @deprecated Use puglUpdate().
*/
static inline PUGL_DEPRECATED_BY("puglUpdate")
PuglStatus
puglPollEvents(PuglWorld* world, double timeout)
{
  return puglUpdate(world, timeout);
}

/**
   Dispatch any pending events to views.

   This processes all pending events, dispatching them to the appropriate
   views.  View event handlers will be called in the scope of this call.  This
   function does not block, if no events are pending then it will return
   immediately.

   @deprecated Use puglUpdate().
*/
static inline PUGL_DEPRECATED_BY("puglUpdate")
PuglStatus
puglDispatchEvents(PuglWorld* world)
{
  return puglUpdate(world, 0.0);
}

static inline PUGL_DEPRECATED_BY("puglShow")
PuglStatus
puglShowWindow(PuglView* view)
{
  return puglShow(view);
}

static inline PUGL_DEPRECATED_BY("puglHide")
PuglStatus
puglHideWindow(PuglView* view)
{
  return puglHide(view);
}

/**
   Set the default size of the view.

   This should be called before puglRealize() to set the default size of the
   view, which will be the initial size of the window if this is a top level
   view.

   @return #PUGL_UNKNOWN_ERROR on failure, but always succeeds if the view is
   not yet realized.
*/
static inline PUGL_DEPRECATED_BY("puglSetSizeHint")
PuglStatus
puglSetDefaultSize(PuglView* view, int width, int height)
{
  return puglSetSizeHint(
    view, PUGL_DEFAULT_SIZE, (PuglSpan)width, (PuglSpan)height);
}

/**
   Set the minimum size of the view.

   If an initial minimum size is known, this should be called before
   puglRealize() to avoid stutter, though it can be called afterwards as well.

   @return #PUGL_UNKNOWN_ERROR on failure, but always succeeds if the view is
   not yet realized.
*/
static inline PUGL_DEPRECATED_BY("puglSetSizeHint")
PuglStatus
puglSetMinSize(PuglView* view, int width, int height)
{
  return puglSetSizeHint(
    view, PUGL_MIN_SIZE, (PuglSpan)width, (PuglSpan)height);
}

/**
   Set the maximum size of the view.

   If an initial maximum size is known, this should be called before
   puglRealize() to avoid stutter, though it can be called afterwards as well.

   @return #PUGL_UNKNOWN_ERROR on failure, but always succeeds if the view is
   not yet realized.
*/
static inline PUGL_DEPRECATED_BY("puglSetSizeHint")
PuglStatus
puglSetMaxSize(PuglView* view, int width, int height)
{
  return puglSetSizeHint(
    view, PUGL_MAX_SIZE, (PuglSpan)width, (PuglSpan)height);
}

/**
   Set the view aspect ratio range.

   The x and y values here represent a ratio of width to height.  To set a
   fixed aspect ratio, set the minimum and maximum values to the same ratio.

   Note that setting different minimum and maximum constraints does not
   currently work on MacOS (the minimum is used), so only setting a fixed
   aspect ratio works properly across all platforms.

   If an initial aspect ratio is known, this should be called before
   puglRealize() to avoid stutter, though it can be called afterwards as well.

   @return #PUGL_UNKNOWN_ERROR on failure, but always succeeds if the view is
   not yet realized.
*/
static inline PUGL_DEPRECATED_BY("puglSetSizeHint")
PuglStatus
puglSetAspectRatio(PuglView* view, int minX, int minY, int maxX, int maxY)
{
  const PuglStatus st0 =
    puglSetSizeHint(view, PUGL_MIN_ASPECT, (PuglSpan)minX, (PuglSpan)minY);

  const PuglStatus st1 =
    puglSetSizeHint(view, PUGL_MAX_ASPECT, (PuglSpan)maxX, (PuglSpan)maxY);

  return st0 ? st0 : st1;
}

/// Return the native window handle
static inline PUGL_DEPRECATED_BY("puglGetNativeView")
PuglNativeView
puglGetNativeWindow(PuglView* view)
{
  return puglGetNativeView(view);
}

#endif // PUGL_DISABLE_DEPRECATED

/**
   @}
   @}
*/

PUGL_END_DECLS

#endif // PUGL_PUGL_H
