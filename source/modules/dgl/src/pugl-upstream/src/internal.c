// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "internal.h"

#include "types.h"

#include "pugl/pugl.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

PuglStatus
puglSetBlob(PuglBlob* const dest, const void* const data, const size_t len)
{
  if (data) {
    void* const newData = realloc(dest->data, len + 1);
    if (!newData) {
      free(dest->data);
      dest->len = 0;
      return PUGL_NO_MEMORY;
    }

    memcpy(newData, data, len);
    ((char*)newData)[len] = 0;

    dest->len  = len;
    dest->data = newData;
  } else {
    dest->len  = 0;
    dest->data = NULL;
  }

  return PUGL_SUCCESS;
}

void
puglSetString(char** dest, const char* string)
{
  if (*dest != string) {
    const size_t len = strlen(string);

    *dest = (char*)realloc(*dest, len + 1);
    strncpy(*dest, string, len + 1);
  }
}

uint32_t
puglDecodeUTF8(const uint8_t* buf)
{
#define FAIL_IF(cond) \
  do {                \
    if (cond)         \
      return 0xFFFD;  \
  } while (0)

  // http://en.wikipedia.org/wiki/UTF-8

  if (buf[0] < 0x80) {
    return buf[0];
  }

  if (buf[0] < 0xC2) {
    return 0xFFFD;
  }

  if (buf[0] < 0xE0) {
    FAIL_IF((buf[1] & 0xC0u) != 0x80);
    return ((uint32_t)buf[0] << 6u) + buf[1] - 0x3080u;
  }

  if (buf[0] < 0xF0) {
    FAIL_IF((buf[1] & 0xC0u) != 0x80);
    FAIL_IF(buf[0] == 0xE0 && buf[1] < 0xA0);
    FAIL_IF((buf[2] & 0xC0u) != 0x80);
    return ((uint32_t)buf[0] << 12u) + //
           ((uint32_t)buf[1] << 6u) +  //
           ((uint32_t)buf[2] - 0xE2080u);
  }

  if (buf[0] < 0xF5) {
    FAIL_IF((buf[1] & 0xC0u) != 0x80);
    FAIL_IF(buf[0] == 0xF0 && buf[1] < 0x90);
    FAIL_IF(buf[0] == 0xF4 && buf[1] >= 0x90);
    FAIL_IF((buf[2] & 0xC0u) != 0x80u);
    FAIL_IF((buf[3] & 0xC0u) != 0x80u);
    return (((uint32_t)buf[0] << 18u) + //
            ((uint32_t)buf[1] << 12u) + //
            ((uint32_t)buf[2] << 6u) +  //
            ((uint32_t)buf[3] - 0x3C82080u));
  }

  return 0xFFFD;
}

PuglStatus
puglDispatchSimpleEvent(PuglView* view, const PuglEventType type)
{
  assert(type == PUGL_CREATE || type == PUGL_DESTROY || type == PUGL_MAP ||
         type == PUGL_UNMAP || type == PUGL_UPDATE || type == PUGL_CLOSE ||
         type == PUGL_LOOP_ENTER || type == PUGL_LOOP_LEAVE);

  const PuglEvent event = {{type, 0}};
  return puglDispatchEvent(view, &event);
}

static inline bool
puglMustConfigure(PuglView* view, const PuglConfigureEvent* configure)
{
  return !!memcmp(configure, &view->lastConfigure, sizeof(PuglConfigureEvent));
}

PuglStatus
puglConfigure(PuglView* view, const PuglEvent* event)
{
  PuglStatus st = PUGL_SUCCESS;

  assert(event->type == PUGL_CONFIGURE);

  view->frame.x      = event->configure.x;
  view->frame.y      = event->configure.y;
  view->frame.width  = event->configure.width;
  view->frame.height = event->configure.height;

  if (puglMustConfigure(view, &event->configure)) {
    st                  = view->eventFunc(view, event);
    view->lastConfigure = event->configure;
  }

  return st;
}

PuglStatus
puglExpose(PuglView* view, const PuglEvent* event)
{
  return (event->expose.width > 0.0 && event->expose.height > 0.0)
           ? view->eventFunc(view, event)
           : PUGL_SUCCESS;
}

PuglStatus
puglDispatchEvent(PuglView* view, const PuglEvent* event)
{
  PuglStatus st0 = PUGL_SUCCESS;
  PuglStatus st1 = PUGL_SUCCESS;

  switch (event->type) {
  case PUGL_NOTHING:
    break;
  case PUGL_CREATE:
  case PUGL_DESTROY:
    if (!(st0 = view->backend->enter(view, NULL))) {
      st0 = view->eventFunc(view, event);
      st1 = view->backend->leave(view, NULL);
    }
    break;
  case PUGL_CONFIGURE:
    if (puglMustConfigure(view, &event->configure)) {
      if (!(st0 = view->backend->enter(view, NULL))) {
        st0 = puglConfigure(view, event);
        st1 = view->backend->leave(view, NULL);
      }
    }
    break;
  case PUGL_MAP:
    if (!view->visible) {
      view->visible = true;
      st0           = view->eventFunc(view, event);
    }
    break;
  case PUGL_UNMAP:
    if (view->visible) {
      view->visible = false;
      st0           = view->eventFunc(view, event);
    }
    break;
  case PUGL_EXPOSE:
    if (!(st0 = view->backend->enter(view, &event->expose))) {
      st0 = puglExpose(view, event);
      st1 = view->backend->leave(view, &event->expose);
    }
    break;
  default:
    st0 = view->eventFunc(view, event);
  }

  return st0 ? st0 : st1;
}
