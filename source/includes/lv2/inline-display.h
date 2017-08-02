/*
  Copyright 2012 David Robillard <http://drobilla.net>
  Copyright 2016 Robin Gareus <robin@gareus.org>

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
   @defgroup inlinedisplay Inline-Display

   Support for displaying a miniaturized, non-interactive view
   in the host's mixer strip.

   @{
*/

#ifndef LV2_INLINE_DISPLAY_H
#define LV2_INLINE_DISPLAY_H

#include <stdint.h>

#include "lv2.h"

#define LV2_INLINEDISPLAY_URI "http://harrisonconsoles.com/lv2/inlinedisplay"
#define LV2_INLINEDISPLAY_PREFIX LV2_INLINEDISPLAY_URI "#"
#define LV2_INLINEDISPLAY__interface LV2_INLINEDISPLAY_PREFIX "interface"
#define LV2_INLINEDISPLAY__queue_draw LV2_INLINEDISPLAY_PREFIX "queue_draw"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque handle for LV2_Inline_Display::queue_draw() */
typedef void* LV2_Inline_Display_Handle;

/** raw image pixmap format is ARGB32,
 * the data pointer is owned by the plugin and must be valid
 * from the first call to render until cleanup.
 */
typedef struct {
	unsigned char *data;
	int width;
	int height;
	int stride;
} LV2_Inline_Display_Image_Surface;

/** a LV2 Feature provided by the Host to the plugin
 *
 * This allows a the plugin during in realtime context to
 * invalidate the currently displayed data and request from
 * the host to call render() as soon as feasible.
 */
typedef struct {
	/** Opaque host data */
	LV2_Inline_Display_Handle handle;
	/** Request from run() that the host should call render() at a later
	 * time to update the inline display
	 */
	void (*queue_draw)(LV2_Inline_Display_Handle handle);
} LV2_Inline_Display;

/**
 * Plugin Inline-Display Interface.
 */
typedef struct {
	/**
	 * The render method. This is called by the host in the main GUI thread
	 * (non realtime, the context with access to graphics drawing APIs).
	 *
	 * The data pointer is owned by the plugin and must be valid
	 * from the first call to render until cleanup.
	 *
	 * The host specifies a maxium available area for the plugin to draw.
	 * the returned Image Surface contains the actual area which
	 * the plugin allocated.
	 *
	 * @param instance The LV2 instance
	 * @param w the max available width
	 * @param h the max available height
	 * @return pointer to a LV2_Inline_Display_Image_Surface or NULL
	 */
	LV2_Inline_Display_Image_Surface* (*render)(
	                                   LV2_Handle instance,
	                                   uint32_t w, uint32_t h);
} LV2_Inline_Display_Interface;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_INLINE_DISPLAY_H */

/**
   @}
*/
