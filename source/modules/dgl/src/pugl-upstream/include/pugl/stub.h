/*
  Copyright 2019-2020 David Robillard <d@drobilla.net>

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

#ifndef PUGL_STUB_H
#define PUGL_STUB_H

#include "pugl/pugl.h"

PUGL_BEGIN_DECLS

/**
   @defgroup stub Stub
   Native graphics support.
   @ingroup pugl
   @{
*/

/**
   Stub graphics backend accessor.

   This backend just creates a simple native window without setting up any
   portable graphics API.
*/
PUGL_CONST_API
const PuglBackend*
puglStubBackend(void);

/**
   @}
*/

PUGL_END_DECLS

#endif // PUGL_STUB_H
