/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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
// Compatibility checks

#if defined(DGL_CAIRO) && defined(DGL_EXTERNAL)
# error invalid build config: trying to build for both cairo and external at the same time
#elif defined(DGL_CAIRO) && defined(DGL_OPENGL)
# error invalid build config: trying to build for both cairo and opengl at the same time
#elif defined(DGL_CAIRO) && defined(DGL_VULKAN)
# error invalid build config: trying to build for both cairo and vulkan at the same time
#elif defined(DGL_EXTERNAL) && defined(DGL_OPENGL)
# error invalid build config: trying to build for both external and opengl at the same time
#elif defined(DGL_EXTERNAL) && defined(DGL_VULKAN)
# error invalid build config: trying to build for both external and vulkan at the same time
#elif defined(DGL_OPENGL) && defined(DGL_VULKAN)
# error invalid build config: trying to build for both opengl and vulkan at the same time
#endif

#ifdef DGL_USE_FILEBROWSER
# error typo detected use DGL_USE_FILE_BROWSER instead of DGL_USE_FILEBROWSER
#endif

#ifdef DGL_USE_WEBVIEW
# error typo detected use DGL_USE_WEB_VIEW instead of DGL_USE_WEBVIEW
#endif

#if defined(DGL_FILE_BROWSER_DISABLED)
# error DGL_FILE_BROWSER_DISABLED has been replaced by DGL_USE_FILE_BROWSER (opt-in vs opt-out)
#endif

#ifndef DGL_ALLOW_DEPRECATED_METHODS
# define DGL_ALLOW_DEPRECATED_METHODS 1
#endif

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
    kModifierShift      = 1U << 0U, ///< Shift key
    kModifierControl    = 1U << 1U, ///< Control key
    kModifierAlt        = 1U << 2U, ///< Alt/Option key
    kModifierSuper      = 1U << 3U, ///< Mod4/Command/Windows key
    kModifierNumLock    = 1U << 4U, ///< Num lock enabled
    kModifierScrollLock = 1U << 5U, ///< Scroll lock enabled
    kModifierCapsLock   = 1U << 6U, ///< Caps lock enabled
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
    kKeyBackspace = 0x00000008U, ///< Backspace
    kKeyTab       = 0x00000009U, ///< Tab
    kKeyEnter     = 0x0000000DU, ///< Enter
    kKeyEscape    = 0x0000001BU, ///< Escape
    kKeyDelete    = 0x0000007FU, ///< Delete
    kKeySpace     = 0x00000020U, ///< Space

    // Unicode Private Use Area
    kKeyF1 = 0xE000U,          ///< F1
    kKeyF2,                    ///< F2
    kKeyF3,                    ///< F3
    kKeyF4,                    ///< F4
    kKeyF5,                    ///< F5
    kKeyF6,                    ///< F6
    kKeyF7,                    ///< F7
    kKeyF8,                    ///< F8
    kKeyF9,                    ///< F9
    kKeyF10,                   ///< F10
    kKeyF11,                   ///< F11
    kKeyF12,                   ///< F12
    kKeyPageUp = 0xE031U,      ///< Page Up
    kKeyPageDown,              ///< Page Down
    kKeyEnd,                   ///< End
    kKeyHome,                  ///< Home
    kKeyLeft,                  ///< Left
    kKeyUp,                    ///< Up
    kKeyRight,                 ///< Right
    kKeyDown,                  ///< Down
    kKeyPrintScreen = 0xE041U, ///< Print Screen
    kKeyInsert,                ///< Insert
    kKeyPause,                 ///< Pause/Break
    kKeyMenu,                  ///< Menu
    kKeyNumLock,               ///< Num Lock
    kKeyScrollLock,            ///< Scroll Lock
    kKeyCapsLock,              ///< Caps Lock
    kKeyShiftL = 0xE051U,      ///< Left Shift
    kKeyShiftR,                ///< Right Shift
    kKeyControlL,              ///< Left Control
    kKeyControlR,              ///< Right Control
    kKeyAltL,                  ///< Left Alt
    kKeyAltR,                  ///< Right Alt / AltGr
    kKeySuperL,                ///< Left Super
    kKeySuperR,                ///< Right Super
    kKeyPad0 = 0xE060U,        ///< Keypad 0
    kKeyPad1,                  ///< Keypad 1
    kKeyPad2,                  ///< Keypad 2
    kKeyPad3,                  ///< Keypad 3
    kKeyPad4,                  ///< Keypad 4
    kKeyPad5,                  ///< Keypad 5
    kKeyPad6,                  ///< Keypad 6
    kKeyPad7,                  ///< Keypad 7
    kKeyPad8,                  ///< Keypad 8
    kKeyPad9,                  ///< Keypad 9
    kKeyPadEnter,              ///< Keypad Enter
    kKeyPadPageUp = 0xE071U,   ///< Keypad Page Up
    kKeyPadPageDown,           ///< Keypad Page Down
    kKeyPadEnd,                ///< Keypad End
    kKeyPadHome,               ///< Keypad Home
    kKeyPadLeft,               ///< Keypad Left
    kKeyPadUp,                 ///< Keypad Up
    kKeyPadRight,              ///< Keypad Right
    kKeyPadDown,               ///< Keypad Down
    kKeyPadClear = 0xE09DU,    ///< Keypad Clear/Begin
    kKeyPadInsert,             ///< Keypad Insert
    kKeyPadDelete,             ///< Keypad Delete
    kKeyPadEqual,              ///< Keypad Equal
    kKeyPadMultiply = 0xE0AAU, ///< Keypad Multiply
    kKeyPadAdd,                ///< Keypad Add
    kKeyPadSeparator,          ///< Keypad Separator
    kKeyPadSubtract,           ///< Keypad Subtract
    kKeyPadDecimal,            ///< Keypad Decimal
    kKeyPadDivide,             ///< Keypad Divide

    // Backwards compatibility with old DPF
    kCharBackspace DISTRHO_DEPRECATED_BY("kKeyBackspace") = kKeyBackspace,
    kCharEscape    DISTRHO_DEPRECATED_BY("kKeyEscape") = kKeyEscape,
    kCharDelete    DISTRHO_DEPRECATED_BY("kKeyDelete") = kKeyDelete,

    kKeyShift   DISTRHO_DEPRECATED_BY("kKeyShiftL") = kKeyShiftL,
    kKeyControl DISTRHO_DEPRECATED_BY("kKeyControlL") = kKeyControlL,
    kKeyAlt     DISTRHO_DEPRECATED_BY("kKeyAltL") = kKeyAltL,
    kKeySuper   DISTRHO_DEPRECATED_BY("kKeySuperL") = kKeySuperL,
};

/**
   Common flags for all events.
 */
enum EventFlag {
    kFlagSendEvent = 1, ///< Event is synthetic
    kFlagIsHint    = 2, ///< Event is a hint (not direct user input)
};

/**
   Reason for a crossing event.
 */
enum CrossingMode {
    kCrossingNormal, ///< Crossing due to pointer motion
    kCrossingGrab,   ///< Crossing due to a grab
    kCrossingUngrab, ///< Crossing due to a grab release
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
    kMouseCursorArrow,           ///< Default pointing arrow
    kMouseCursorCaret,           ///< Caret (I-Beam) for text entry
    kMouseCursorCrosshair,       ///< Cross-hair
    kMouseCursorHand,            ///< Hand with a pointing finger
    kMouseCursorNotAllowed,      ///< Operation not allowed
    kMouseCursorLeftRight,       ///< Left/right arrow for horizontal resize
    kMouseCursorUpDown,          ///< Up/down arrow for vertical resize
    kMouseCursorUpLeftDownRight, ///< Diagonal arrow for down/right resize
    kMouseCursorUpRightDownLeft, ///< Diagonal arrow for down/left resize
    kMouseCursorAllScroll,       ///< Omnidirectional "arrow" for scrolling

    // Backwards compatibility with old DPF
    kMouseCursorDiagonal     DISTRHO_DEPRECATED_BY("kMouseCursorUpLeftDownRight") = kMouseCursorUpLeftDownRight,
    kMouseCursorAntiDiagonal DISTRHO_DEPRECATED_BY("kMouseCursorUpRightDownLeft") = kMouseCursorUpRightDownLeft,
};

/**
   Scroll direction.

   Describes the direction of a scroll event along with whether the scroll is a "smooth" scroll.
   The discrete directions are for devices like mouse wheels with constrained axes,
   while a smooth scroll is for those with arbitrary scroll direction freedom, like some touchpads.
*/
enum ScrollDirection {
    kScrollUp,     ///< Scroll up
    kScrollDown,   ///< Scroll down
    kScrollLeft,   ///< Scroll left
    kScrollRight,  ///< Scroll right
    kScrollSmooth, ///< Smooth scroll in any direction
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
