/*
  Copyright 2014 David Robillard <http://drobilla.net>

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

#ifndef PUGL_COMMON_H_INCLUDED
#define PUGL_COMMON_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/**
   @addtogroup pugl
   @{
*/

/**
   A Pugl view.
*/
typedef struct PuglViewImpl PuglView;

/**
   A native window handle.

   On X11, this is a Window.
   On OSX, this is an NSView*.
   On Windows, this is a HWND.
*/
typedef intptr_t PuglNativeWindow;

/**
   Handle for opaque user data.
*/
typedef void* PuglHandle;

/**
   Return status code.
*/
typedef enum {
	PUGL_SUCCESS = 0
} PuglStatus;

/**
   Drawing context type.
*/
typedef enum {
	PUGL_GL,
	PUGL_CAIRO
} PuglContextType;

/**
   Convenience symbols for ASCII control characters.
*/
typedef enum {
	PUGL_CHAR_BACKSPACE = 0x08,
	PUGL_CHAR_ESCAPE    = 0x1B,
	PUGL_CHAR_DELETE    = 0x7F
} PuglChar;

/**
   Keyboard modifier flags.
*/
typedef enum {
	PUGL_MOD_SHIFT = 1 << 0,  /**< Shift key */
	PUGL_MOD_CTRL  = 1 << 1,  /**< Control key */
	PUGL_MOD_ALT   = 1 << 2,  /**< Alt/Option key */
	PUGL_MOD_SUPER = 1 << 3   /**< Mod4/Command/Windows key */
} PuglMod;

/**
   Special (non-Unicode) keyboard keys.
*/
typedef enum {
	PUGL_KEY_F1 = 1,
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
	PUGL_KEY_CTRL,
	PUGL_KEY_ALT,
	PUGL_KEY_SUPER
} PuglKey;

/**
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* PUGL_COMMON_H_INCLUDED */
