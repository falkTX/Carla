/*
  Copyright 2012-2014 David Robillard <http://drobilla.net>

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
   @file pugl.h API for Pugl, a minimal portable API for OpenGL.
*/

#ifndef PUGL_H_INCLUDED
#define PUGL_H_INCLUDED

#include <stdint.h>

#include "pugl/common.h"
#include "pugl/event.h"

#ifdef PUGL_SHARED
#    ifdef _WIN32
#        define PUGL_LIB_IMPORT __declspec(dllimport)
#        define PUGL_LIB_EXPORT __declspec(dllexport)
#    else
#        define PUGL_LIB_IMPORT __attribute__((visibility("default")))
#        define PUGL_LIB_EXPORT __attribute__((visibility("default")))
#    endif
#    ifdef PUGL_INTERNAL
#        define PUGL_API PUGL_LIB_EXPORT
#    else
#        define PUGL_API PUGL_LIB_IMPORT
#    endif
#else
#    ifdef _WIN32
#        define PUGL_API
#    else
#        define PUGL_API __attribute__((visibility("hidden")))
#    endif
#endif

#ifdef __cplusplus
extern "C" {
#else
#    include <stdbool.h>
#endif

/**
   @defgroup pugl Pugl
   A minimal portable API for OpenGL.
   @{
*/

/**
   A function called when the window is closed.
*/
typedef void (*PuglCloseFunc)(PuglView* view);

/**
   A function called to draw the view contents with OpenGL.
*/
typedef void (*PuglDisplayFunc)(PuglView* view);

/**
   A function called when a key is pressed or released.
   @param view The view the event occured in.
   @param press True if the key was pressed, false if released.
   @param key Unicode point of the key pressed.
   @return 0 if event was handled, otherwise send event to parent window.
*/
typedef int (*PuglKeyboardFunc)(PuglView* view, bool press, uint32_t key);

/**
   A function called when the pointer moves.
   @param view The view the event occured in.
   @param x The window-relative x coordinate of the pointer.
   @param y The window-relative y coordinate of the pointer.
*/
typedef void (*PuglMotionFunc)(PuglView* view, int x, int y);

/**
   A function called when a mouse button is pressed or released.
   @param view The view the event occured in.
   @param button The button number (1 = left, 2 = middle, 3 = right).
   @param press True if the key was pressed, false if released.
   @param x The window-relative x coordinate of the pointer.
   @param y The window-relative y coordinate of the pointer.
*/
typedef void (*PuglMouseFunc)(
	PuglView* view, int button, bool press, int x, int y);

/**
   A function called when the view is resized.
   @param view The view being resized.
   @param width The new view width.
   @param height The new view height.
*/
typedef void (*PuglReshapeFunc)(PuglView* view, int width, int height);

/**
   A function called on scrolling (e.g. mouse wheel or track pad).

   The distances used here are in "lines", a single tick of a clicking mouse
   wheel.  For example, @p dy = 1.0 scrolls 1 line up.  Some systems and
   devices support finer resolution and/or higher values for fast scrolls,
   so programs should handle any value gracefully.

   @param view The view being scrolled.
   @param x The window-relative x coordinate of the pointer.
   @param y The window-relative y coordinate of the pointer.
   @param dx The scroll x distance.
   @param dx The scroll y distance.
*/
typedef void (*PuglScrollFunc)(PuglView* view, int x, int y, float dx, float dy);

/**
   A function called when a special key is pressed or released.

   This callback allows the use of keys that do not have unicode points.
   Note that some are non-printable keys.

   @param view The view the event occured in.
   @param press True if the key was pressed, false if released.
   @param key The key pressed.
   @return 0 if event was handled, otherwise send event to parent window.
*/
typedef int (*PuglSpecialFunc)(PuglView* view, bool press, PuglKey key);

/**
   A function called when a filename is selected via file-browser.

   @param view The view the event occured in.
   @param filename The selected file name or NULL if the dialog was canceled.
*/
typedef void (*PuglFileSelectedFunc)(PuglView* view, const char* filename);

/**
   @name Initialization
   Configuration functions which must be called before creating a window.
   @{
*/

/**
   Create a Pugl context.

   To create a window, call the various puglInit* functions as necessary, then
   call puglCreateWindow().
*/
PUGL_API PuglView*
puglInit(void);

/**
   Set the parent window before creating a window (for embedding).
*/
PUGL_API void
puglInitWindowParent(PuglView* view, PuglNativeWindow parent);

/**
   Set the window size before creating a window.
*/
PUGL_API void
puglInitWindowSize(PuglView* view, int width, int height);

/**
   Set the minimum window size before creating a window.
*/
PUGL_API void
puglInitWindowMinSize(PuglView* view, int width, int height);

/**
   Enable or disable resizing before creating a window.
*/
PUGL_API void
puglInitUserResizable(PuglView* view, bool resizable);

/**
   Set transient parent before creating a window.

   On X11, parent_id must be a Window.
   On OSX, parent_id must be an NSView*.
*/
PUGL_API void
puglInitTransientFor(PuglView* view, uintptr_t parent);

/**
   Set the context type before creating a window.
*/
PUGL_API void
puglInitContextType(PuglView* view, PuglContextType type);

/**
   @}
*/

/**
   @name Windows
   Window management functions.
   @{
*/

/**
   Create a window with the settings given by the various puglInit functions.

   @return 1 (pugl does not currently support multiple windows).
*/
PUGL_API int
puglCreateWindow(PuglView* view, const char* title);

/**
   Show the current window.
*/
PUGL_API void
puglShowWindow(PuglView* view);

/**
   Hide the current window.
*/
PUGL_API void
puglHideWindow(PuglView* view);

/**
   Return the native window handle.
*/
PUGL_API PuglNativeWindow
puglGetNativeWindow(PuglView* view);

/**
   @}
*/

/**
   Set the handle to be passed to all callbacks.

   This is generally a pointer to a struct which contains all necessary state.
   Everything needed in callbacks should be here, not in static variables.

   Note the lack of this facility makes GLUT unsuitable for plugins or
   non-trivial programs; this mistake is largely why Pugl exists.
*/
PUGL_API void
puglSetHandle(PuglView* view, PuglHandle handle);

/**
   Get the handle to be passed to all callbacks.
*/
PUGL_API PuglHandle
puglGetHandle(PuglView* view);

/**
   Get the drawing context.

   For PUGL_GL contexts, this is unused and returns NULL.
   For PUGL_CAIRO contexts, this returns a pointer to a cairo_t.
*/
PUGL_API void*
puglGetContext(PuglView* view);

/**
   Return the timestamp (if any) of the currently-processing event.
*/
PUGL_API uint32_t
puglGetEventTimestamp(PuglView* view);

/**
   Get the currently active modifiers (PuglMod flags).

   This should only be called from an event handler.
*/
PUGL_API int
puglGetModifiers(PuglView* view);

/**
   Ignore synthetic repeated key events.
*/
PUGL_API void
puglIgnoreKeyRepeat(PuglView* view, bool ignore);

/**
   @name Event Callbacks
   Functions to set event callbacks for handling user input.
   @{
*/

/**
   Set the function to call when the window is closed.
*/
PUGL_API void
puglSetCloseFunc(PuglView* view, PuglCloseFunc closeFunc);

/**
   Set the display function which should draw the UI using GL.
*/
PUGL_API void
puglSetDisplayFunc(PuglView* view, PuglDisplayFunc displayFunc);

/**
   Set the function to call on keyboard events.
*/
PUGL_API void
puglSetKeyboardFunc(PuglView* view, PuglKeyboardFunc keyboardFunc);

/**
   Set the function to call on mouse motion.
*/
PUGL_API void
puglSetMotionFunc(PuglView* view, PuglMotionFunc motionFunc);

/**
   Set the function to call on mouse button events.
*/
PUGL_API void
puglSetMouseFunc(PuglView* view, PuglMouseFunc mouseFunc);

/**
   Set the function to call on scroll events.
*/
PUGL_API void
puglSetScrollFunc(PuglView* view, PuglScrollFunc scrollFunc);

/**
   Set the function to call on special events.
*/
PUGL_API void
puglSetSpecialFunc(PuglView* view, PuglSpecialFunc specialFunc);

/**
   Set the function to call when the window size changes.
*/
PUGL_API void
puglSetReshapeFunc(PuglView* view, PuglReshapeFunc reshapeFunc);

/**
   Set the function to call on file-browser selections.
*/
PUGL_API void
puglSetFileSelectedFunc(PuglView* view, PuglFileSelectedFunc fileSelectedFunc);

/**
   @}
*/

/**
   Grab the input focus.
*/
PUGL_API void
puglGrabFocus(PuglView* view);

/**
   Process all pending window events.

   This handles input events as well as rendering, so it should be called
   regularly and rapidly enough to keep the UI responsive.
*/
PUGL_API PuglStatus
puglProcessEvents(PuglView* view);

/**
   Request a redisplay on the next call to puglProcessEvents().
*/
PUGL_API void
puglPostRedisplay(PuglView* view);

/**
   Destroy a GL window.
*/
PUGL_API void
puglDestroy(PuglView* view);

/**
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* PUGL_H_INCLUDED */
