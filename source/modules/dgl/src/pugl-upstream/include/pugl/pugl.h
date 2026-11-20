// Copyright 2012-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_PUGL_H
#define PUGL_PUGL_H

#include <pugl/attributes.h>

#include <stddef.h>
#include <stdint.h>

#ifndef __cplusplus
#  include <stdbool.h>
#endif

PUGL_BEGIN_DECLS

/**
   @defgroup pugl Pugl C API
   @{
*/

/**
   @defgroup pugl_geometry_types Geometry Types
   @{
*/

/**
   A pixel coordinate within/of a view.

   This is used to represent coordinates in two different spaces, depending on
   context: parent-relative and view-relative.

   Parent-relative coordinates of a view are relative to the top-left of the
   parent it's embedded in, or to that of a system-specific root.  Top-level
   positions should be between -16000 and 16000.

   View-relative coordinates in a view are relative to the top-left of the
   view.
*/
typedef int16_t PuglCoord;

/**
   A pixel span (width or height) within/of a view.

   The width and height of a view should be between 1 and 10000.
*/
typedef uint16_t PuglSpan;

/// A 2-dimensional position within/of a view
typedef struct {
  PuglCoord x;
  PuglCoord y;
} PuglPoint;

/// A 2-dimensional size within/of a view
typedef struct {
  PuglSpan width;
  PuglSpan height;
} PuglArea;

/**
   @}
   @defgroup pugl_events Events
   @{
*/

/// The type of a #PuglEvent
typedef enum {
  PUGL_NOTHING,        ///< No event
  PUGL_REALIZE,        ///< View realized, a #PuglRealizeEvent
  PUGL_UNREALIZE,      ///< View unrealized, a #PuglUnrealizeEvent
  PUGL_CONFIGURE,      ///< View configured, a #PuglConfigureEvent
  PUGL_UPDATE,         ///< View update before draw, a #PuglUpdateEvent
  PUGL_EXPOSE,         ///< View expose during draw, a #PuglExposeEvent
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
  PUGL_IS_SEND_EVENT = 1U << 0U, ///< Event is synthetic
  PUGL_IS_HINT       = 1U << 1U, ///< Event is a hint (not direct user input)
} PuglEventFlag;

/// Bitwise OR of #PuglEventFlag values
typedef uint32_t PuglEventFlags;

/// Common header for all events
typedef struct {
  PuglEventType  type;  ///< Event type
  PuglEventFlags flags; ///< Bitwise OR of #PuglEventFlag values
} PuglAnyEvent;

/**
   @defgroup pugl_management_events Management Events
   @{
*/

/**
   View style flags.

   Style flags reflect special modes and states supported by the window system.
   Applications can monitor or manipulate them to improve integration.
*/
typedef enum {
  /// View is mapped to a real window and potentially visible
  PUGL_VIEW_STYLE_MAPPED = 1U << 0U,

  /// View is modal, typically a dialog box of its transient parent
  PUGL_VIEW_STYLE_MODAL = 1U << 1U,

  /// View should be above most others
  PUGL_VIEW_STYLE_ABOVE = 1U << 2U,

  /// View should be below most others
  PUGL_VIEW_STYLE_BELOW = 1U << 3U,

  /// View is minimized, shaded, or otherwise invisible
  PUGL_VIEW_STYLE_HIDDEN = 1U << 4U,

  /// View is maximized to fill the screen vertically
  PUGL_VIEW_STYLE_TALL = 1U << 5U,

  /// View is maximized to fill the screen horizontally
  PUGL_VIEW_STYLE_WIDE = 1U << 6U,

  /// View is enlarged to fill the entire screen with no decorations
  PUGL_VIEW_STYLE_FULLSCREEN = 1U << 7U,

  /// View is being resized
  PUGL_VIEW_STYLE_RESIZING = 1U << 8U,

  /// View is ready for input or otherwise demanding attention
  PUGL_VIEW_STYLE_DEMANDING = 1U << 9U,
} PuglViewStyleFlag;

/// The maximum #PuglViewStyleFlag value
#define PUGL_MAX_VIEW_STYLE_FLAG PUGL_VIEW_STYLE_DEMANDING

/// Bitwise OR of #PuglViewStyleFlag values
typedef uint32_t PuglViewStyleFlags;

/**
   View realize event.

   This is sent when a view is realized before it's first displayed, with the
   graphics context entered (but not prepared to draw).  This can be used to
   set up the graphics system, for example by loading OpenGL extensions.
*/
typedef PuglAnyEvent PuglRealizeEvent;

/**
   View unrealize event.

   This counterpart to #PuglRealizeEvent is sent when the view will no longer
   be displayed.  This can be used to tear down the graphics system, or free
   any resources allocated when the view was realized.
*/
typedef PuglAnyEvent PuglUnrealizeEvent;

/**
   View configure (resize/move) event.

   This is sent whenever the view is resized or moved.  The graphics context
   isn't active during a configure event, so these values should be saved if
   necessary and applied the next time the view is exposed.
*/
typedef struct {
  PuglEventType      type;   ///< #PUGL_CONFIGURE
  PuglEventFlags     flags;  ///< Bitwise OR of #PuglEventFlag values
  PuglCoord          x;      ///< Parent-relative X coordinate of view
  PuglCoord          y;      ///< Parent-relative Y coordinate of view
  PuglSpan           width;  ///< Width of view
  PuglSpan           height; ///< Height of view
  PuglViewStyleFlags style;  ///< Bitwise OR of #PuglViewStyleFlag flags
} PuglConfigureEvent;

/**
   Recursive loop enter event.

   This is sent when the window system enters a recursive loop.  While in the
   recursive loop, the main loop will be stalled and no expose events will be
   received.  A typical use is to install a timer to perform continuous
   actions, such as drawing.

   - MacOS: A recursive loop is entered while the window is being live resized.

   - Windows: A recursive loop is entered while the window is being live
     resized or the menu is shown.

   - X11: A recursive loop is never entered and the event loop runs as usual
     while the view is being resized.
*/
typedef PuglAnyEvent PuglLoopEnterEvent;

/**
   Recursive loop leave event.

   This is sent after a loop enter event when the recursive loop is finished
   and normal iteration will continue.
*/
typedef PuglAnyEvent PuglLoopLeaveEvent;

/**
   View close event.

   This is sent when the view is to be closed, for example when the user clicks
   the close button.
*/
typedef PuglAnyEvent PuglCloseEvent;

/**
   @}
   @defgroup pugl_update_events Update Events
   @{
*/

/**
   View update event.

   This is sent near the end of a main loop iteration when the view is
   potentially about to be redrawn.  It can be used to mark regions for
   exposure with puglObscureView() or puglObscureRegion().  For example, to
   continuously animate, obscure the view whenever an update event is received,
   then receive and handle the expose event(s) shortly afterwards.
*/
typedef PuglAnyEvent PuglUpdateEvent;

/**
   Expose event requiring a redraw.

   This is sent when a region needs to be redrawn, with the graphics context
   entered and activated for drawing.  The application must completely draw the
   given region.
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
   @}
   @defgroup pugl_keyboard_events Keyboard Events
   @{
*/

/**
   Keyboard key codepoints.

   All keys are identified by a Unicode code point in #PuglKeyEvent::key.  This
   enumeration defines constants for special keys that don't have a standard
   code point, and some convenience constants for control characters.  Note
   that all keys are handled in the same way, this enumeration is just for
   convenience when writing hard-coded key bindings.

   Keys that don't have a standard code point use values in the Private Use
   Area in the Basic Multilingual Plane (`U+E000` to `U+F8FF`).  Applications
   must take care to not interpret these values beyond key detection, the
   mapping used here is arbitrary and specific to Pugl.
*/
typedef enum {
  PUGL_KEY_NONE          = 0U,      ///< Sentinel value for no key
  PUGL_KEY_BACKSPACE     = 0x0008U, ///< Backspace
  PUGL_KEY_TAB           = 0x0009U, ///< Tab
  PUGL_KEY_ENTER         = 0x000DU, ///< Enter
  PUGL_KEY_ESCAPE        = 0x001BU, ///< Escape
  PUGL_KEY_DELETE        = 0x007FU, ///< Delete
  PUGL_KEY_SPACE         = 0x0020U, ///< Space
  PUGL_KEY_F1            = 0xE000U, ///< F1
  PUGL_KEY_F2            = 0xE001U, ///< F2
  PUGL_KEY_F3            = 0xE002U, ///< F3
  PUGL_KEY_F4            = 0xE003U, ///< F4
  PUGL_KEY_F5            = 0xE004U, ///< F5
  PUGL_KEY_F6            = 0xE005U, ///< F6
  PUGL_KEY_F7            = 0xE006U, ///< F7
  PUGL_KEY_F8            = 0xE007U, ///< F8
  PUGL_KEY_F9            = 0xE008U, ///< F9
  PUGL_KEY_F10           = 0xE009U, ///< F10
  PUGL_KEY_F11           = 0xE010U, ///< F11
  PUGL_KEY_F12           = 0xE011U, ///< F12
  PUGL_KEY_PAGE_UP       = 0xE031U, ///< Page Up
  PUGL_KEY_PAGE_DOWN     = 0xE032U, ///< Page Down
  PUGL_KEY_END           = 0xE033U, ///< End
  PUGL_KEY_HOME          = 0xE034U, ///< Home
  PUGL_KEY_LEFT          = 0xE035U, ///< Left
  PUGL_KEY_UP            = 0xE036U, ///< Up
  PUGL_KEY_RIGHT         = 0xE037U, ///< Right
  PUGL_KEY_DOWN          = 0xE038U, ///< Down
  PUGL_KEY_PRINT_SCREEN  = 0xE041U, ///< Print Screen
  PUGL_KEY_INSERT        = 0xE042U, ///< Insert
  PUGL_KEY_PAUSE         = 0xE043U, ///< Pause/Break
  PUGL_KEY_MENU          = 0xE044U, ///< Menu
  PUGL_KEY_NUM_LOCK      = 0xE045U, ///< Num Lock
  PUGL_KEY_SCROLL_LOCK   = 0xE046U, ///< Scroll Lock
  PUGL_KEY_CAPS_LOCK     = 0xE047U, ///< Caps Lock
  PUGL_KEY_SHIFT_L       = 0xE051U, ///< Left Shift
  PUGL_KEY_SHIFT_R       = 0xE052U, ///< Right Shift
  PUGL_KEY_CTRL_L        = 0xE053U, ///< Left Control
  PUGL_KEY_CTRL_R        = 0xE054U, ///< Right Control
  PUGL_KEY_ALT_L         = 0xE055U, ///< Left Alt
  PUGL_KEY_ALT_R         = 0xE056U, ///< Right Alt / AltGr
  PUGL_KEY_SUPER_L       = 0xE057U, ///< Left Super
  PUGL_KEY_SUPER_R       = 0xE058U, ///< Right Super
  PUGL_KEY_PAD_0         = 0xE060U, ///< Keypad 0
  PUGL_KEY_PAD_1         = 0xE061U, ///< Keypad 1
  PUGL_KEY_PAD_2         = 0xE062U, ///< Keypad 2
  PUGL_KEY_PAD_3         = 0xE063U, ///< Keypad 3
  PUGL_KEY_PAD_4         = 0xE064U, ///< Keypad 4
  PUGL_KEY_PAD_5         = 0xE065U, ///< Keypad 5
  PUGL_KEY_PAD_6         = 0xE066U, ///< Keypad 6
  PUGL_KEY_PAD_7         = 0xE067U, ///< Keypad 7
  PUGL_KEY_PAD_8         = 0xE068U, ///< Keypad 8
  PUGL_KEY_PAD_9         = 0xE069U, ///< Keypad 9
  PUGL_KEY_PAD_ENTER     = 0xE070U, ///< Keypad Enter
  PUGL_KEY_PAD_PAGE_UP   = 0xE071U, ///< Keypad Page Up
  PUGL_KEY_PAD_PAGE_DOWN = 0xE072U, ///< Keypad Page Down
  PUGL_KEY_PAD_END       = 0xE073U, ///< Keypad End
  PUGL_KEY_PAD_HOME      = 0xE074U, ///< Keypad Home
  PUGL_KEY_PAD_LEFT      = 0xE075U, ///< Keypad Left
  PUGL_KEY_PAD_UP        = 0xE076U, ///< Keypad Up
  PUGL_KEY_PAD_RIGHT     = 0xE077U, ///< Keypad Right
  PUGL_KEY_PAD_DOWN      = 0xE078U, ///< Keypad Down
  PUGL_KEY_PAD_CLEAR     = 0xE09DU, ///< Keypad Clear/Begin
  PUGL_KEY_PAD_INSERT    = 0xE09EU, ///< Keypad Insert
  PUGL_KEY_PAD_DELETE    = 0xE09FU, ///< Keypad Delete
  PUGL_KEY_PAD_EQUAL     = 0xE0A0U, ///< Keypad Equal
  PUGL_KEY_PAD_MULTIPLY  = 0xE0AAU, ///< Keypad Multiply
  PUGL_KEY_PAD_ADD       = 0xE0ABU, ///< Keypad Add
  PUGL_KEY_PAD_SEPARATOR = 0xE0ACU, ///< Keypad Separator
  PUGL_KEY_PAD_SUBTRACT  = 0xE0ADU, ///< Keypad Subtract
  PUGL_KEY_PAD_DECIMAL   = 0xE0AEU, ///< Keypad Decimal
  PUGL_KEY_PAD_DIVIDE    = 0xE0AFU, ///< Keypad Divide
} PuglKey;

/// Keyboard modifier flags
typedef enum {
  PUGL_MOD_SHIFT       = 1U << 0U, ///< Shift pressed
  PUGL_MOD_CTRL        = 1U << 1U, ///< Control pressed
  PUGL_MOD_ALT         = 1U << 2U, ///< Alt/Option pressed
  PUGL_MOD_SUPER       = 1U << 3U, ///< Super/Command/Windows pressed
  PUGL_MOD_NUM_LOCK    = 1U << 4U, ///< Num lock enabled
  PUGL_MOD_SCROLL_LOCK = 1U << 5U, ///< Scroll lock enabled
  PUGL_MOD_CAPS_LOCK   = 1U << 6U, ///< Caps lock enabled
} PuglMod;

/// Bitwise OR of #PuglMod values
typedef uint32_t PuglMods;

/// Reason for a PuglCrossingEvent
typedef enum {
  PUGL_CROSSING_NORMAL, ///< Crossing due to pointer motion
  PUGL_CROSSING_GRAB,   ///< Crossing due to a grab
  PUGL_CROSSING_UNGRAB, ///< Crossing due to a grab release
} PuglCrossingMode;

/**
   Keyboard focus event.

   This is sent whenever the view gains or loses the keyboard focus.  The view
   with the keyboard focus will receive any key press or release events.
*/
typedef struct {
  PuglEventType    type;  ///< #PUGL_FOCUS_IN or #PUGL_FOCUS_OUT
  PuglEventFlags   flags; ///< Bitwise OR of #PuglEventFlag values
  PuglCrossingMode mode;  ///< Reason for focus change
} PuglFocusEvent;

/**
   Key press or release event.

   This represents low-level key presses and releases.  This can be used for
   "direct" keyboard handling like key bindings, but must not be interpreted as
   text input.

   Keys are represented portably as Unicode code points, using the "natural"
   code point for the key where possible (see #PuglKey for details).  The `key`
   field is the code for the pressed key, without any modifiers applied.  For
   example, a press or release of the 'A' key will have `key` 97 ('a')
   regardless of whether shift or control are being held.

   Alternatively, the raw `keycode` can be used to work directly with physical
   keys, but note that this value isn't portable and differs between platforms
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

   This represents text input, usually as the result of a key press.  The text
   is given both as a Unicode character code and a UTF-8 string.

   Note that this event is generated by the platform's input system, so there
   isn't necessarily a direct correspondence between text events and physical
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
   @}
   @defgroup pugl_pointer_events Pointer Events
   @{
*/

/**
   Scroll direction.

   Describes the direction of a #PuglScrollEvent along with whether the scroll
   is a "smooth" scroll.  The discrete directions are for devices like mouse
   wheels with constrained axes, while a smooth scroll is for those with
   arbitrary scroll direction freedom, like some touchpads.
*/
typedef enum {
  PUGL_SCROLL_UP,     ///< Scroll up
  PUGL_SCROLL_DOWN,   ///< Scroll down
  PUGL_SCROLL_LEFT,   ///< Scroll left
  PUGL_SCROLL_RIGHT,  ///< Scroll right
  PUGL_SCROLL_SMOOTH, ///< Smooth scroll in any direction
} PuglScrollDirection;

/**
   Pointer enter or leave event.

   This is sent when the pointer enters or leaves the view.  This can happen
   for several reasons (not just the user dragging the pointer over the window
   edge), as described by the `mode` field.
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
   @}
   @defgroup pugl_custom_events Custom Events
   @{
*/

/**
   Custom client message event.

   This is a custom event sent to a view with puglSendEvent(), which wakes up
   the event loop and delivers two pointer-sized data fields.  These
   client-specific fields are opaque to Pugl and aren't interpreted in any way.
*/
typedef struct {
  PuglEventType  type;  ///< #PUGL_CLIENT
  PuglEventFlags flags; ///< Bitwise OR of #PuglEventFlag values
  uintptr_t      data1; ///< Client-specific data
  uintptr_t      data2; ///< Client-specific data
} PuglClientEvent;

/**
   Timer event.

   This is sent while the timer with the given `id` is activated at the regular
   interval specified in the corresponding puglStartTimer() call.  Other
   platform-specific timers may exist, so applications must always check the
   `id` field before reacting to a timer event.
*/
typedef struct {
  PuglEventType  type;  ///< #PUGL_TIMER
  PuglEventFlags flags; ///< Bitwise OR of #PuglEventFlag values
  uintptr_t      id;    ///< Timer ID
} PuglTimerEvent;

/**
   @}
   @defgroup pugl_clipboard_events Clipboard Events
   @{
*/

/**
   Clipboard data offer event.

   This is sent when a clipboard has data present, possibly with several
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

   This is sent after accepting a data offer when the data has been retrieved
   and converted.  While handling this event, the data can be accessed with
   puglGetClipboard().
*/
typedef struct {
  PuglEventType  type;      ///< #PUGL_DATA
  PuglEventFlags flags;     ///< Bitwise OR of #PuglEventFlag values
  double         time;      ///< Time in seconds
  uint32_t       typeIndex; ///< Index of datatype
} PuglDataEvent;

/**
   @}
*/

/**
   View event.

   This is a union of all event types.  The `type` must be checked to determine
   which fields are safe to access.  A pointer to PuglEvent can either be cast
   to the appropriate type, or the union members used.

   The graphics system may only be accessed when handling certain events.  The
   graphics context is active for #PUGL_REALIZE, #PUGL_UNREALIZE, and
   #PUGL_EXPOSE, but only enabled for drawing for #PUGL_EXPOSE.
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
   @defgroup pugl_status Status
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
  PUGL_BAD_CALL,              ///< Invalid call
  PUGL_BACKEND_FAILED,        ///< Backend initialization failed
  PUGL_REGISTRATION_FAILED,   ///< Class registration failed
  PUGL_REALIZE_FAILED,        ///< System view realization failed
  PUGL_SET_FORMAT_FAILED,     ///< Failed to set pixel format
  PUGL_CREATE_CONTEXT_FAILED, ///< Failed to create drawing context
  PUGL_UNSUPPORTED,           ///< Unsupported operation
  PUGL_NO_MEMORY,             ///< Failed to allocate memory
} PuglStatus;

/// Return a string describing a status code
PUGL_CONST_API const char*
puglStrerror(PuglStatus status);

/**
   @}
   @defgroup pugl_string_hints String Hints
   @{
*/

/// A string property for configuration
typedef enum {
  /**
     The application name.

     This is a stable identifier for the application, usually something like
     "my_app" which is the same as the name of the executable.  It's used as
     the applications name for X11 resources.
  */
  PUGL_APPLICATION_NAME,

  /**
     The application or window class name.

     This is a stable identifier for the application window class, a short
     camel-case name like "MyApp".  This should be the same for every instance
     of the application, but different from any other application.  On X11 and
     Windows, it's used to set the class name of windows, which is used for
     window management and configuration.
  */
  PUGL_CLASS_NAME,

  /**
     The title of the window or application.

     This is used by the system to display a title for the application or
     window, for example in title bars or window/application switchers.  It's
     only used to display a label to the user, not as an identifier, and can
     change over time to reflect the current state of the application.  For
     example, it's common for programs to add the name of the current
     document, like "myfile.txt - Fancy Editor".
  */
  PUGL_WINDOW_TITLE,
} PuglStringHint;

/// The number of #PuglStringHint values
#define PUGL_NUM_STRING_HINTS 3U

/**
   @}
   @defgroup pugl_world World
   @{
*/

/**
   The "world" of application state.

   The world represents everything that isn't associated with a particular
   view.  Several worlds can be created in a single process, but code using
   different worlds must be isolated so they are never mixed.  Views are
   strongly associated with the world they were created in.
*/
typedef struct PuglWorldImpl PuglWorld;

/// The type of a World
typedef enum {
  PUGL_PROGRAM, ///< Top-level application
  PUGL_MODULE,  ///< Plugin or module within a larger application
} PuglWorldType;

/// World flags
typedef enum {
  /**
     Set up support for threads if necessary.

     X11: Calls XInitThreads() which is required for some drivers.
  */
  PUGL_WORLD_THREADS = 1U << 0U,
} PuglWorldFlag;

/// Bitwise OR of #PuglWorldFlag values
typedef uint32_t PuglWorldFlags;

/**
   Create a new world.

   @param type The type, which dictates what this world is responsible for.
   @param flags Flags to control world features.
   @return A new world, which must be later freed with puglFreeWorld().
*/
PUGL_MALLOC_API PuglWorld*
puglNewWorld(PuglWorldType type, PuglWorldFlags flags);

/// Free a world allocated with puglNewWorld()
PUGL_API void
puglFreeWorld(PuglWorld* world);

/**
   Set the user data for the world.

   This is usually a pointer to a struct that contains all the state shared by
   any views.

   The handle is opaque to Pugl and isn't interpreted in any way.
*/
PUGL_API void
puglSetWorldHandle(PuglWorld* world, void* handle);

/// Get the user data for the world
PUGL_API void*
puglGetWorldHandle(PuglWorld* world);

/**
   Return the native world handle.

   The "native world" is a system-specific handle that's shared across an
   entire program or module.  It serves as a common denominator between Pugl,
   applications, and the windowing system API.

   X11: Returns a pointer to the `Display`.

   MacOS: Returns a pointer to the `NSApplication`.

   Windows: Returns the `HMODULE` of the calling process.
*/
PUGL_API void*
puglGetNativeWorld(PuglWorld* world);

/**
   Set a string property to configure the world or application.

   The string value only needs to be valid for the duration of this call, it
   will be copied if necessary.
*/
PUGL_API PuglStatus
puglSetWorldString(PuglWorld* world, PuglStringHint key, const char* value);

/**
   Get a world or application string property.

   The returned string should be accessed immediately or copied if necessary,
   it may be invalidated by any change to the same world.
*/
PUGL_API const char*
puglGetWorldString(const PuglWorld* world, PuglStringHint key);

/**
   Return the time in seconds.

   This is a monotonically increasing clock with high resolution.  The returned
   time is only useful to compare against other times returned by this
   function, its absolute value has no meaning.
*/
PUGL_API double
puglGetTime(const PuglWorld* world);

/**
   Update by processing events from the window system.

   This runs a single iteration of the main loop, and needs to be called
   repeatedly to update all views.

   Plugins should always use a timeout of zero to avoid blocking the host.  For
   continuously animating programs, a timeout that's a reasonable fraction of
   the ideal frame period should be used, to minimize input latency by ensuring
   that as many input events are consumed as possible before drawing.

   @param world The world to update.

   @param timeout If zero, then this call won't block.  If positive, then
   events will be processed for that amount of time, starting from when this
   function was called.  If negative, then this call will block until an event
   occurs.

   @return #PUGL_SUCCESS if events are read, #PUGL_FAILURE if no events are
   read, or an error.
*/
PUGL_API PuglStatus
puglUpdate(PuglWorld* world, double timeout);

/**
   @}
   @defgroup pugl_backend Backend
   @{
*/

/**
   A graphics backend.

   The backend dictates how graphics are set up for a view, and how drawing is
   performed.  A backend must be set by calling puglSetBackend() before
   realizing a view.
*/
typedef struct PuglBackendImpl PuglBackend;

/**
   @}
   @defgroup pugl_view View
   @{
*/

/**
   A drawable region that receives events.

   A view can be thought of as a window, but doesn't necessarily correspond to
   a top-level window in a desktop environment.  For example, a view can be
   embedded in another, or represent a simple embedded system with no concept
   of windows at all.
*/
typedef struct PuglViewImpl PuglView;

/**
   A native view handle.

   A "native view" is a system-specific handle for a view (or "window").  A
   #PuglView is itself a native view, which may be embedded in another.  Like
   the native world handle, this serves as a common denominator between this
   and other APIs.

   X11: This is a `Window`.

   MacOS: This is a pointer to an `NSView*`.

   Windows: This is a `HWND`.
*/
typedef uintptr_t PuglNativeView;

/// An integer hint for configuring a view
typedef enum {
  PUGL_CONTEXT_API,           ///< OpenGL render API (GL/GLES)
  PUGL_CONTEXT_VERSION_MAJOR, ///< OpenGL context major version
  PUGL_CONTEXT_VERSION_MINOR, ///< OpenGL context minor version
  PUGL_CONTEXT_PROFILE,       ///< OpenGL context profile (core/compatibility)
  PUGL_CONTEXT_DEBUG,         ///< OpenGL context debugging enabled
  PUGL_RED_BITS,              ///< Number of bits for red channel
  PUGL_GREEN_BITS,            ///< Number of bits for green channel
  PUGL_BLUE_BITS,             ///< Number of bits for blue channel
  PUGL_ALPHA_BITS,            ///< Number of bits for alpha channel
  PUGL_DEPTH_BITS,            ///< Number of bits for depth buffer
  PUGL_STENCIL_BITS,          ///< Number of bits for stencil buffer
  PUGL_SAMPLE_BUFFERS,        ///< Number of sample buffers (AA)
  PUGL_SAMPLES,               ///< Number of samples per pixel (AA)
  PUGL_DOUBLE_BUFFER,         ///< True if double buffering should be used
  PUGL_SWAP_INTERVAL,         ///< Number of frames between buffer swaps
  PUGL_RESIZABLE,             ///< True if view should be resizable
  PUGL_IGNORE_KEY_REPEAT,     ///< True if key repeat events are ignored
  PUGL_REFRESH_RATE,          ///< Refresh rate in Hz
  PUGL_VIEW_TYPE,             ///< View type (a #PuglViewType)
  PUGL_DARK_FRAME,            ///< True if window frame should be dark
} PuglViewHint;

/// The number of #PuglViewHint values
#define PUGL_NUM_VIEW_HINTS 20U

/// A special view hint value
typedef enum {
  PUGL_DONT_CARE                    = -1, ///< Generic trinary: unset
  PUGL_FALSE                        = 0,  ///< Generic trinary: false
  PUGL_TRUE                         = 1,  ///< Generic trinary: true
  PUGL_OPENGL_API                   = 2,  ///< For #PUGL_CONTEXT_API
  PUGL_OPENGL_ES_API                = 3,  ///< For #PUGL_CONTEXT_API
  PUGL_OPENGL_CORE_PROFILE          = 4,  ///< For #PUGL_CONTEXT_PROFILE
  PUGL_OPENGL_COMPATIBILITY_PROFILE = 5,  ///< For #PUGL_CONTEXT_PROFILE
} PuglViewHintValue;

/// View type
typedef enum {
  PUGL_VIEW_TYPE_NORMAL,  ///< A normal top-level window
  PUGL_VIEW_TYPE_UTILITY, ///< A utility window like a palette or toolbox
  PUGL_VIEW_TYPE_DIALOG,  ///< A dialog window
} PuglViewType;

/**
   A hint for configuring or constraining the position of a view.

   The system will attempt to make the view adhere to these, but they are
   suggestions, not hard constraints.  Applications should handle any view
   position gracefully.

   An unset position has `INT16_MIN` (-32768) for both `x` and `y`.  In
   practice, set positions should be between -16000 and 16000 for portability.
   Usually, the origin is the top left of the display, although negative
   coordinates are possible, particularly on multi-display systems.
*/
typedef enum {
  /**
     Default position.

     This is used as the initial position if no other position is specified.
     It isn't necessary to set a default position (unlike the default size,
     which is required).  If not even a default position is set, then the
     window will be placed somewhere reasonable according to platform
     conventions.  Applications must not make assumptions about the initial
     position, which is implementation-defined, platform-specific, and for
     embedded views, may become invalid if the parent's size is adjusted.
  */
  PUGL_DEFAULT_POSITION,

  /**
     Current position.

     This reflects the current position of the view, which may be different
     from the default position if the view has been moved by the user, window
     manager, or for any other reason.  It overrides the default position.
  */
  PUGL_CURRENT_POSITION,
} PuglPositionHint;

/// The number of #PuglPositionHint values
#define PUGL_NUM_POSITION_HINTS 2U

/**
   A hint for configuring/constraining the size of a view.

   The system will attempt to make the view adhere to these, but they are
   suggestions, not hard constraints.  Applications should handle any view size
   gracefully.
*/
typedef enum {
  /**
     Default size.

     This is used as the size during window creation as a default, if no other
     size is specified.
  */
  PUGL_DEFAULT_SIZE,

  /**
     Current size.

     This reflects the current size of the view, which may be different from
     the default size if the view is resizable.  It overrides the default size.
  */
  PUGL_CURRENT_SIZE,

  /**
     Minimum size.

     If set, the view should be at least this large.
  */
  PUGL_MIN_SIZE,

  /**
     Maximum size.

     If set, the view should be at most this large.
  */
  PUGL_MAX_SIZE,

  /**
     Fixed aspect ratio.

     If set, the view should have this aspect ratio.  Mutually exclusive with
     #PUGL_MIN_ASPECT and #PUGL_MAX_ASPECT.
  */
  PUGL_FIXED_ASPECT,

  /**
     Minimum aspect ratio.

     If set, the view should have an aspect ratio no lower than this.  Mutually
     exclusive with #PUGL_FIXED_ASPECT.
  */
  PUGL_MIN_ASPECT,

  /**
     Maximum aspect ratio.

     If set, the view should have an aspect ratio no higher than this.
     Mutually exclusive with #PUGL_FIXED_ASPECT.
  */
  PUGL_MAX_ASPECT,
} PuglSizeHint;

/// The number of #PuglSizeHint values
#define PUGL_NUM_SIZE_HINTS 7U

/// A function called when an event occurs
typedef PuglStatus (*PuglEventFunc)(PuglView* view, const PuglEvent* event);

/**
   @defgroup pugl_setup Setup
   Functions for creating and destroying a view.
   @{
*/

/**
   Create a new view.

   A newly created view doesn't correspond to a real system view or window
   until it's configured and realized.
*/
PUGL_MALLOC_API PuglView*
puglNewView(PuglWorld* world);

/// Free a view created with puglNewView()
PUGL_API void
puglFreeView(PuglView* view);

/// Return the world that `view` is a part of
PUGL_API PuglWorld*
puglGetWorld(PuglView* view);

/**
   Set the user data for a view.

   This is usually a pointer to a struct that contains all the state needed by
   a view.  Everything needed to process events should be stored here, not in
   static variables.

   The handle is opaque to Pugl and isn't interpreted in any way.
*/
PUGL_API void
puglSetHandle(PuglView* view, void* handle);

/// Get the user data for a view
PUGL_API void*
puglGetHandle(PuglView* view);

/**
   Set the graphics backend to use for a view.

   This must be called once to set the graphics backend before calling
   puglRealize().

   Pugl includes a "stub" backend for platform-specific applications, and
   several portable backends:

   - puglCairoBackend()
   - puglGlBackend()
   - puglVulkanBackend()

   These backends are modules that aren't compiled in to the main Pugl library
   to avoid unnecessary dependencies.  To use a particular backend,
   applications must link against the appropriate backend library, or be sure
   to compile in the appropriate code if using a local copy of Pugl.
*/
PUGL_API PuglStatus
puglSetBackend(PuglView* view, const PuglBackend* backend);

/// Return the graphics backend used by a view
PUGL_API const PuglBackend*
puglGetBackend(const PuglView* view);

/// Set the function to call when an event occurs
PUGL_API PuglStatus
puglSetEventFunc(PuglView* view, PuglEventFunc eventFunc);

/**
   Set a hint to configure view properties.

   This only has an effect when called before puglRealize().
*/
PUGL_API PuglStatus
puglSetViewHint(PuglView* view, PuglViewHint hint, int value);

/**
   Get the value for a view hint.

   If the view has been realized, this can be used to get the actual value of a
   hint which was initially set to PUGL_DONT_CARE, or has been adjusted from
   the suggested value.
*/
PUGL_API int
puglGetViewHint(const PuglView* view, PuglViewHint hint);

/**
   Set a string property to configure view properties.

   This is similar to puglSetViewHint() but sets hints with string values.  The
   string value only needs to be valid for the duration of this call, it will
   be copied if necessary.
*/
PUGL_API PuglStatus
puglSetViewString(PuglView* view, PuglStringHint key, const char* value);

/**
   Get a view string property.

   The returned string should be accessed immediately, or copied.  It may
   become invalid upon any call to any function that manipulates the same view.
*/
PUGL_API const char*
puglGetViewString(const PuglView* view, PuglStringHint key);

/**
   Return the scale factor of the view.

   This factor describe how large UI elements (especially text) should be
   compared to "normal".  For example, 2.0 means the UI should be drawn twice
   as large.  "Normal" is loosely defined as a typical default size on a
   "standard DPI" display (around 96 DPI).
*/
PUGL_API double
puglGetScaleFactor(const PuglView* view);

/**
   @}
   @defgroup pugl_frame Frame
   Functions for working with the position and size of a view.
   @{
*/

/**
   Get a position hint for the view.

   This can be used to get the default or current position of a view, in screen
   coordinates with an upper left origin.
*/
PUGL_API PuglPoint
puglGetPositionHint(const PuglView* view, PuglPositionHint hint);

/**
   Set a position hint for the view.

   This can be used to set the default or current position of a view.

   This should be called before puglRealize() so the initial window for the
   view can be configured correctly.  It may also be used dynamically after the
   window is realized, for some hints.

   @return An error code on failure, but always succeeds if the view isn't yet
   realized.
*/
PUGL_API PuglStatus
puglSetPositionHint(PuglView* view, PuglPositionHint hint, int x, int y);

/**
   Get a size hint for the view.

   This can be used to get the default, current, minimum, and maximum size of a
   view, as well as the supported range of aspect ratios.
*/
PUGL_API PuglArea
puglGetSizeHint(const PuglView* view, PuglSizeHint hint);

/**
   Set a size hint for the view.

   This can be used to set the default, current, minimum, and maximum size of a
   view, as well as the supported range of aspect ratios.

   This should be called before puglRealize() so the initial window for the
   view can be configured correctly.  It may also be used dynamically after the
   window is realized, for some hints.

   @return An error code on failure, but always succeeds if the view isn't yet
   realized.
*/
PUGL_API PuglStatus
puglSetSizeHint(PuglView*    view,
                PuglSizeHint hint,
                unsigned     width,
                unsigned     height);

/**
   @}
   @defgroup pugl_window Window
   Functions to control the top-level window of a view.
   @{
*/

/**
   Set the native view the view will be embedded in.

   This must be called before puglRealize(), reparenting isn't supported.
*/
PUGL_API PuglStatus
puglSetParent(PuglView* view, PuglNativeView parent);

/**
   Return the native view that the view is embedded in.

   @return The native view handle of the parent that `view` is embedded in, or
   zero (if view is top-level or unrealized).
*/
PUGL_API PuglNativeView
puglGetParent(const PuglView* view);

/**
   Set the transient parent of the window.

   Set this for transient children like dialogs, to have them properly
   associated with their parent window.  This should be called before
   puglRealize().

   A view can either have a parent (for embedding) or a transient parent (for
   top-level windows like dialogs), but not both.
*/
PUGL_API PuglStatus
puglSetTransientParent(PuglView* view, PuglNativeView parent);

/**
   Return the transient parent of the window.

   @return The native handle to the window this view is a transient child of,
   or null.
*/
PUGL_API PuglNativeView
puglGetTransientParent(const PuglView* view);

/**
   Realize a view by creating a corresponding system view or window.

   After this call, the (initially invisible) underlying system view exists and
   can be accessed with puglGetNativeView().

   The view should be fully configured using the above functions before this is
   called.  This function may only be called once per view.
*/
PUGL_API PuglStatus
puglRealize(PuglView* view);

/**
   Unrealize a view by destroying the corresponding system view or window.

   This is the inverse of puglRealize().  After this call, the view no longer
   corresponds to a real system view, and can be realized again later.
*/
PUGL_API PuglStatus
puglUnrealize(PuglView* view);

/// A command to control the behaviour of puglShow()
typedef enum {
  /**
     Realize and show the window without intentionally raising it.

     This will weakly "show" the window but without making any effort to raise
     it.  Depending on the platform or system configuration, the window may be
     raised above some others regardless.
  */
  PUGL_SHOW_PASSIVE,

  /**
     Raise the window to the top of the application's stack.

     This is the normal "well-behaved" way to show and raise the window, which
     should be used in most cases.
  */
  PUGL_SHOW_RAISE,

  /**
     Aggressively force the window to be raised to the top.

     This will attempt to raise the window to the top, even if this isn't the
     active application, or if doing so would otherwise go against the
     platform's guidelines.  This generally shouldn't be used, and isn't
     guaranteed to work.  On modern Windows systems, the active application
     must explicitly grant permission for others to steal the foreground from
     it.
  */
  PUGL_SHOW_FORCE_RAISE,
} PuglShowCommand;

/**
   Show the view.

   If the view has not yet been realized, the first call to this function will
   do so automatically.  If the view is currently hidden, it will be shown and
   possibly raised to the top depending on the platform.

   @return #PUGL_SUCCESS, an error from realization, or #PUGL_FAILURE if the
   window was shown but not raised.
*/
PUGL_API PuglStatus
puglShow(PuglView* view, PuglShowCommand command);

/**
   Hide the view.

   This is the counterpart to puglShow().  If the view is currently visible, it
   will be hidden, otherwise, this does nothing.
*/
PUGL_API PuglStatus
puglHide(PuglView* view);

/**
   Set a view style, if supported by the system.

   This can be used to manipulate the window into various special states, but
   note that not all states are supported on all systems.  This function may
   return failure or an error if the platform implementation doesn't
   "understand" how to set the given style, but the return value here can't be
   used to determine if the state has actually been set.  Any changes to the
   actual state of the view will arrive in later configure events.
*/
PUGL_API PuglStatus
puglSetViewStyle(PuglView* view, PuglViewStyleFlags flags);

/// Return the style flags currently set for a view
PUGL_API PuglViewStyleFlags
puglGetViewStyle(const PuglView* view);

/// Return true iff the view is currently visible
PUGL_API bool
puglGetVisible(const PuglView* view);

/// Return the native window handle
PUGL_API PuglNativeView
puglGetNativeView(const PuglView* view);

/**
   @}
   @defgroup pugl_graphics Graphics
   Functions for working with the graphics context and scheduling redisplays.
   @{
*/

/**
   Get the graphics context.

   This is a backend-specific context used for drawing if the backend graphics
   API requires one.  It's only available during an expose.

   Cairo: Returns a pointer to a
   [cairo_t](http://www.cairographics.org/manual/cairo-cairo-t.html).

   All other backends: returns null.
*/
PUGL_API void*
puglGetContext(PuglView* view);

/**
   Request a redisplay for the entire view.

   This will cause an expose event to be dispatched later.  If called from
   within the event handler, the expose should arrive at the end of the current
   event loop iteration, though this isn't strictly guaranteed on all
   platforms.  If called elsewhere, an expose will be enqueued to be processed
   in the next event loop iteration.
*/
PUGL_API PuglStatus
puglObscureView(PuglView* view);

/**
   "Obscure" a region so it will be exposed in the next render.

   This will cause an expose event to be dispatched later.  If called from
   within the event handler, the expose should arrive at the end of the current
   event loop iteration, though this isn't strictly guaranteed on all
   platforms.  If called elsewhere, an expose will be enqueued to be processed
   in the next event loop iteration.

   The region is clamped to the size of the view if necessary.

   @param view The view to expose later.
   @param x The top-left X coordinate of the rectangle to obscure.
   @param y The top-left Y coordinate of the rectangle to obscure.
   @param width The width of the rectangle to obscure.
   @param height The height coordinate of the rectangle to obscure.
*/
PUGL_API PuglStatus
puglObscureRegion(PuglView* view,
                  int       x,
                  int       y,
                  unsigned  width,
                  unsigned  height);

/**
   @}
   @defgroup pugl_interaction Interaction
   Functions for interacting with the user and window system.
   @{
*/

/**
   A mouse cursor type.

   This is a portable subset of mouse cursors that exist on X11, MacOS, and
   Windows.
*/
typedef enum {
  PUGL_CURSOR_ARROW,              ///< Default pointing arrow
  PUGL_CURSOR_CARET,              ///< Caret (I-Beam) for text entry
  PUGL_CURSOR_CROSSHAIR,          ///< Cross-hair
  PUGL_CURSOR_HAND,               ///< Hand with a pointing finger
  PUGL_CURSOR_NO,                 ///< Operation not allowed
  PUGL_CURSOR_LEFT_RIGHT,         ///< Left/right arrow for horizontal resize
  PUGL_CURSOR_UP_DOWN,            ///< Up/down arrow for vertical resize
  PUGL_CURSOR_UP_LEFT_DOWN_RIGHT, ///< Diagonal arrow for down/right resize
  PUGL_CURSOR_UP_RIGHT_DOWN_LEFT, ///< Diagonal arrow for down/left resize
  PUGL_CURSOR_ALL_SCROLL,         ///< Omnidirectional "arrow" for scrolling
} PuglCursor;

/// The number of #PuglCursor values
#define PUGL_NUM_CURSORS 10U

/**
   Grab the keyboard input focus.

   Note that this will fail if the view isn't mapped and so shouldn't, for
   example, be called immediately after puglShow().

   @return #PUGL_SUCCESS if the focus was successfully grabbed, or an error.
*/
PUGL_API PuglStatus
puglGrabFocus(PuglView* view);

/**
   Return whether `view` has the keyboard input focus.

   @return True if the view is realized and focused for keyboard input.
*/
PUGL_API bool
puglHasFocus(const PuglView* view);

/**
   Request data from the general copy/paste clipboard.

   A #PUGL_DATA_OFFER event will be sent if data is available.
*/
PUGL_API PuglStatus
puglPaste(PuglView* view);

/**
   Return the number of types available for the data in a clipboard.

   Returns zero if the clipboard is empty.
*/
PUGL_API uint32_t
puglGetNumClipboardTypes(const PuglView* view);

/**
   Return the identifier of a type available in a clipboard.

   This is usually a MIME type, but may also be another platform-specific type
   identifier.  Applications must ignore any type they don't recognize.

   Returns null if `typeIndex` is out of bounds according to
   puglGetNumClipboardTypes().
*/
PUGL_API const char*
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
PUGL_API PuglStatus
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
PUGL_API PuglStatus
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
PUGL_API const void*
puglGetClipboard(PuglView* view, uint32_t typeIndex, size_t* len);

/**
   Set the mouse cursor.

   This changes the system cursor that is displayed when the pointer is inside
   the view.  May fail if setting the cursor isn't supported on this system,
   for example if compiled on X11 without Xcursor support.

   @return #PUGL_BAD_PARAMETER if the given cursor is invalid,
   #PUGL_UNSUPPORTED if setting the cursor isn't supported on this system, or
   another error if the cursor is known but loading it fails.
*/
PUGL_API PuglStatus
puglSetCursor(PuglView* view, PuglCursor cursor);

/**
   Activate a repeating timer event.

   This starts a timer which will send a #PuglTimerEvent to `view` every
   `timeout` seconds.  This can be used to perform some action in a view at a
   regular interval with relatively low frequency.  Note that the frequency of
   timer events may be limited by how often puglUpdate() is called.

   If the given timer already exists, it is replaced.

   @param view The view to begin sending #PUGL_TIMER events to.

   @param id The identifier for this timer.  This is an application-specific
   ID, usually a small positive integer enumerator.  Only a limited number of
   timers is supported, and each adds overhead, so applications should create
   only a few timers, and perform several tasks in each if necessary.

   @param timeout The period, in seconds, of this timer.  The maximum supported
   resolution is about 10 ms on Windows, and about 1 ms on X11 and MacOS.

   @return #PUGL_FAILURE if timers aren't supported by the system,
   #PUGL_UNKNOWN_ERROR if setting the timer failed.
*/
PUGL_API PuglStatus
puglStartTimer(PuglView* view, uintptr_t id, double timeout);

/**
   Stop an active timer.

   @param view The view that the timer is set for.
   @param id The ID previously passed to puglStartTimer().

   @return #PUGL_FAILURE if timers aren't supported by this system,
   #PUGL_UNKNOWN_ERROR if stopping the timer failed.
*/
PUGL_API PuglStatus
puglStopTimer(PuglView* view, uintptr_t id);

/**
   Send an event to a view via the window system.

   If supported, the event will be delivered to the view via the event loop
   like any other.  Only a few event types are currently supported.

   Opaque #PUGL_CLIENT events are supported on all platforms.

   X11: A #PUGL_EXPOSE event can be sent, which is similar to calling
   puglObscureRegion(), but will always send a message to the X server, even
   when called in an event handler.

   @return #PUGL_UNSUPPORTED if sending this type of event isn't supported in
   general, #PUGL_FAILURE if the event can't be sent now.
*/
PUGL_API PuglStatus
puglSendEvent(PuglView* view, const PuglEvent* event);

/**
   @}
   @}
   @}
*/

PUGL_END_DECLS

#endif // PUGL_PUGL_H
