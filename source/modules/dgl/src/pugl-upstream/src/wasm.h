// Copyright 2012-2022 David Robillard <d@drobilla.net>
// Copyright 2021-2022 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_WASM_H
#define PUGL_SRC_WASM_H

// #include "attributes.h"
#include "types.h"

#include "pugl/pugl.h"

struct PuglTimer {
  PuglView* view;
  uintptr_t id;
};

struct PuglWorldInternalsImpl {
  double scaleFactor;
};

struct PuglInternalsImpl {
  PuglSurface* surface;
  bool needsRepaint;
  bool pointerLocked;
  uint32_t numTimers;
  long lastX, lastY;
  double lockedX, lockedY;
  double lockedRootX, lockedRootY;
  struct PuglTimer* timers;
};

#endif // PUGL_SRC_WASM_H
