// Copyright 2012-2022 David Robillard <d@drobilla.net>
// Copyright 2021-2022 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_WASM_H
#define PUGL_SRC_WASM_H

// #include "attributes.h"
#include "types.h"

#include "pugl/pugl.h"

#include <emscripten/emscripten.h>

struct PuglTimer {
  PuglView* view;
  uintptr_t id;
  int timeout;
};

struct PuglWorldInternalsImpl {
  double scaleFactor;
};

struct PuglInternalsImpl {
  PuglSurface* surface;
  bool needsRepaint;
  uint32_t numTimers;
  struct PuglTimer* timers;
};

#endif // PUGL_SRC_WASM_H
