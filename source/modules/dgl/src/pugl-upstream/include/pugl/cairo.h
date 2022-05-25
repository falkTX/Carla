/*
  Copyright 2012-2020 David Robillard <d@drobilla.net>

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

#ifndef PUGL_CAIRO_H
#define PUGL_CAIRO_H

#include "pugl/pugl.h"

PUGL_BEGIN_DECLS

/**
   @defgroup cairo Cairo
   Cairo graphics support.
   @ingroup pugl
   @{
*/

/**
   Cairo graphics backend accessor.

   Pass the returned value to puglSetBackend() to draw to a view with Cairo.
*/
PUGL_CONST_API
const PuglBackend*
puglCairoBackend(void);

/**
   @}
*/

PUGL_END_DECLS

#endif // PUGL_CAIRO_H
