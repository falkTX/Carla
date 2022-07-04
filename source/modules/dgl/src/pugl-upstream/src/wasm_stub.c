// Copyright 2012-2022 David Robillard <d@drobilla.net>
// Copyright 2021-2022 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#include "pugl/stub.h"

#include "stub.h"
// #include "types.h"
// #include "wasm.h"

#include "pugl/pugl.h"

const PuglBackend*
puglStubBackend(void)
{
  static const PuglBackend backend = {
    puglStubConfigure,
    puglStubCreate,
    puglStubDestroy,
    puglStubEnter,
    puglStubLeave,
    puglStubGetContext,
  };

  return &backend;
}
