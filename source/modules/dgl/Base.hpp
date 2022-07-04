/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DGL_BASE_HPP_INCLUDED
#define DGL_BASE_HPP_INCLUDED

#include "../distrho/extra/LeakDetector.hpp"
#include "../distrho/extra/ScopedPointer.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Define namespace

#ifndef DGL_NAMESPACE
# define DGL_NAMESPACE DGL
#endif

#define START_NAMESPACE_DGL namespace DGL_NAMESPACE {
#define END_NAMESPACE_DGL }
#define USE_NAMESPACE_DGL using namespace DGL_NAMESPACE;

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// Base DGL enums

/**
   Keyboard modifier flags.
 */
enum Modifier {
    kModifierShift   = 1u << 0u, ///< Shift key
    kModifierControl = 1u << 1u, ///< Control key
    kModifierAlt     = 1u << 2u, ///< Alt/Option key
    kModifierSuper   = 1u << 3u  ///< Mod4/Command/Windows key
};

/**
   Keyboard key codepoints.

   All keys are identified by a Unicode code point in Widget::KeyboardEvent::key.
   This enumeration defines constants for special keys that do not have a standard
   code point, and some convenience constants for control characters.
   Note that all keys are handled in the same way, this enumeration is just for
   convenience when writing hard-coded key bindings.

   Keys that do not have a standard code point use values in the Private Use
   Area in the Basic Multilingual Plane (`U+E000` to `U+F8FF`).
   Applications must take care to not interpret these values beyond key detection,
   the mapping used here is arbitrary and specific to DPF.
 */
enum Key {
    // Convenience symbols for ASCII control characters
    kKeyBackspace = 0x08,
    kKeyEscape    = 0x1B,
    kKeyDelete    = 0x7F,

    // Backwards compatibility with old DPF
    kCharBackspace DISTRHO_DEPRECATED_BY("kKeyBackspace") = kKeyBackspace,
    kCharEscape    DISTRHO_DEPRECATED_BY("kKeyEscape") = kKeyEscape,
    kCharDelete    DISTRHO_DEPRECATED_BY("kKeyDelete") = kKeyDelete,

    // Unicode Private Use Area
    kKeyF1 = 0xE000,
    kKeyF2,
    kKeyF3,
    kKeyF4,
    kKeyF5,
    kKeyF6,
    kKeyF7,
    kKeyF8,
    kKeyF9,
    kKeyF10,
    kKeyF11,
    kKeyF12,
    kKeyLeft,
    kKeyUp,
    kKeyRight,
    kKeyDown,
    kKeyPageUp,
    kKeyPageDown,
    kKeyHome,
    kKeyEnd,
    kKeyInsert,
    kKeyShift,
    kKeyShiftL = kKeyShift,
    kKeyShiftR,
    kKeyControl,
    kKeyControlL = kKeyControl,
    kKeyControlR,
    kKeyAlt,
    kKeyAltL = kKeyAlt,
    kKeyAltR,
    kKeySuper,
    kKeySuperL = kKeySuper,
    kKeySuperR,
    kKeyMenu,
    kKeyCapsLock,
    kKeyScrollLock,
    kKeyNumLock,
    kKeyPrintScreen,
    kKeyPause
};

/**
   Common flags for all events.
 */
enum EventFlag {
    kFlagSendEvent = 1, ///< Event is synthetic
    kFlagIsHint    = 2  ///< Event is a hint (not direct user input)
};

/**
   Reason for a crossing event.
 */
enum CrossingMode {
    kCrossingNormal, ///< Crossing due to pointer motion
    kCrossingGrab,   ///< Crossing due to a grab
    kCrossingUngrab  ///< Crossing due to a grab release
};

/**
   A mouse button.

   Mouse button numbers start from 1, and are ordered: primary, secondary, middle.
   So, on a typical right-handed mouse, the button numbers are:

   Left: 1
   Right: 2
   Middle (often a wheel): 3

   Higher button numbers are reported in the same order they are represented on the system.
   There is no universal standard here, but buttons 4 and 5 are typically a pair of buttons or a rocker,
   which are usually bound to "back" and "forward" operations.

   Note that these numbers may differ from those used on the underlying
   platform, since they are manipulated to provide a consistent portable API.
*/
enum MouseButton {
    kMouseButtonLeft = 1,
    kMouseButtonRight,
    kMouseButtonMiddle,
};

/**
   A mouse cursor type.

   This is a portable subset of mouse cursors that exist on X11, MacOS, and Windows.
*/
enum MouseCursor {
    kMouseCursorArrow,       ///< Default pointing arrow
    kMouseCursorCaret,       ///< Caret (I-Beam) for text entry
    kMouseCursorCrosshair,   ///< Cross-hair
    kMouseCursorHand,        ///< Hand with a pointing finger
    kMouseCursorNotAllowed,  ///< Operation not allowed
    kMouseCursorLeftRight,   ///< Left/right arrow for horizontal resize
    kMouseCursorUpDown,      ///< Up/down arrow for vertical resize
    kMouseCursorDiagonal,    ///< Top-left to bottom-right arrow for diagonal resize
    kMouseCursorAntiDiagonal ///< Bottom-left to top-right arrow for diagonal resize
};

/**
   Scroll direction.

   Describes the direction of a scroll event along with whether the scroll is a "smooth" scroll.
   The discrete directions are for devices like mouse wheels with constrained axes,
   while a smooth scroll is for those with arbitrary scroll direction freedom, like some touchpads.
*/
enum ScrollDirection {
    kScrollUp,    ///< Scroll up
    kScrollDown,  ///< Scroll down
    kScrollLeft,  ///< Scroll left
    kScrollRight, ///< Scroll right
    kScrollSmooth ///< Smooth scroll in any direction
};

/**
   A clipboard data offer.
   @see Window::onClipboardDataOffer
*/
struct ClipboardDataOffer {
   /**
      The id of this data offer.
      @note The value 0 is reserved for null/invalid.
    */
    uint32_t id;

   /**
      The type of this data offer.
      Usually a MIME type, but may also be another platform-specific type identifier.
    */
    const char* type;
};

// --------------------------------------------------------------------------------------------------------------------
// Base DGL classes

/**
   Graphics context, definition depends on build type.
 */
struct GraphicsContext {};

/**
   Idle callback.
 */
struct IdleCallback
{
    virtual ~IdleCallback() {}
    virtual void idleCallback() = 0;
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#ifndef DONT_SET_USING_DGL_NAMESPACE
  // If your code uses a lot of DGL classes, then this will obviously save you
  // a lot of typing, but can be disabled by setting DONT_SET_USING_DGL_NAMESPACE.
  using namespace DGL_NAMESPACE;
#endif

// --------------------------------------------------------------------------------------------------------------------

#endif // DGL_BASE_HPP_INCLUDED
