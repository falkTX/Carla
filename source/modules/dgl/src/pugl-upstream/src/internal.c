// Copyright 2012-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "internal.h"

#include "types.h"

#include <pugl/pugl.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static PuglPoint
make_point(const PuglCoord x, const PuglCoord y)
{
  const PuglPoint point = {x, y};
  return point;
}

bool
puglIsValidPosition(const int x, const int y)
{
  // INT16_MIN is a sentinel, INT16_MAX is impossible with non-zero size
  return x > INT16_MIN && x < INT16_MAX && y > INT16_MIN && y < INT16_MAX;
}

bool
puglIsValidSize(const unsigned width, const unsigned height)
{
  return width && height && width <= INT16_MAX && height <= INT16_MAX;
}

bool
puglIsValidArea(const PuglArea size)
{
  return size.width && size.height;
}

PuglArea
puglGetInitialSize(const PuglView* const view)
{
  if (view->lastConfigure.type == PUGL_CONFIGURE) {
    // Use the last configured size
    const PuglConfigureEvent config = view->lastConfigure;
    const PuglArea           size   = {config.width, config.height};
    return size;
  }

  // Use the default size hint set by the application
  return view->sizeHints[PUGL_DEFAULT_SIZE];
}

PuglPoint
puglGetInitialPosition(const PuglView* const view, const PuglArea size)
{
  if (view->lastConfigure.type == PUGL_CONFIGURE) {
    // Use the last configured frame
    return make_point(view->lastConfigure.x, view->lastConfigure.y);
  }

  const PuglPoint defaultPos = view->positionHints[PUGL_DEFAULT_POSITION];
  if (puglIsValidPosition(defaultPos.x, defaultPos.y)) {
    // Use the default position hint set by the application
    return make_point(defaultPos.x, defaultPos.y);
  }

  if (view->parent) {
    // Default to the top/left origin of the parent
    return make_point(0, 0);
  }

  // Center frame on a transient ancestor, or failing that, the screen
  const PuglPoint center = puglGetAncestorCenter(view);
  const PuglPoint pos    = {(PuglCoord)(center.x - (size.width / 2)),
                            (PuglCoord)(center.y - (size.height / 2))};
  return pos;
}

void
puglEnsureHint(PuglView* const view, const PuglViewHint hint, const int value)
{
  if (view->hints[hint] == PUGL_DONT_CARE) {
    view->hints[hint] = value;
  }
}

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

PuglStatus
puglSetString(char** dest, const char* string)
{
  if (*dest != string) {
    const size_t len = string ? strlen(string) : 0U;
    if (!len) {
      free(*dest);
      *dest = NULL;
      return PUGL_SUCCESS;
    }

    char* const new_dest = (char*)realloc(*dest, len + 1U);
    if (!new_dest) {
      return PUGL_NO_MEMORY;
    }

    *dest = new_dest;
    memcpy(*dest, string, len + 1U);
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglStoreSizeHint(PuglView* const    view,
                  const PuglSizeHint hint,
                  const unsigned     width,
                  const unsigned     height)
{
  if (view->world->state == PUGL_WORLD_EXPOSING) {
    return PUGL_BAD_CALL;
  }

  if (!puglIsValidSize(width, height)) {
    return PUGL_BAD_PARAMETER;
  }

  view->sizeHints[hint].width  = (PuglSpan)width;
  view->sizeHints[hint].height = (PuglSpan)height;
  return PUGL_SUCCESS;
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
    FAIL_IF((buf[1] & 0xC0U) != 0x80);
    return ((uint32_t)buf[0] << 6U) + buf[1] - 0x3080U;
  }

  if (buf[0] < 0xF0) {
    FAIL_IF((buf[1] & 0xC0U) != 0x80);
    FAIL_IF(buf[0] == 0xE0 && buf[1] < 0xA0);
    FAIL_IF((buf[2] & 0xC0U) != 0x80);
    return ((uint32_t)buf[0] << 12U) + //
           ((uint32_t)buf[1] << 6U) +  //
           ((uint32_t)buf[2] - 0xE2080U);
  }

  if (buf[0] < 0xF5) {
    FAIL_IF((buf[1] & 0xC0U) != 0x80);
    FAIL_IF(buf[0] == 0xF0 && buf[1] < 0x90);
    FAIL_IF(buf[0] == 0xF4 && buf[1] >= 0x90);
    FAIL_IF((buf[2] & 0xC0U) != 0x80U);
    FAIL_IF((buf[3] & 0xC0U) != 0x80U);
    return (((uint32_t)buf[0] << 18U) + //
            ((uint32_t)buf[1] << 12U) + //
            ((uint32_t)buf[2] << 6U) +  //
            ((uint32_t)buf[3] - 0x3C82080U));
  }

  return 0xFFFD;
}

PuglMods
puglFilterMods(const PuglMods state, const PuglKey key)
{
  switch (key) {
  case PUGL_KEY_SHIFT_L:
  case PUGL_KEY_SHIFT_R:
    return state & ~(PuglMods)PUGL_MOD_SHIFT;
  case PUGL_KEY_CTRL_L:
  case PUGL_KEY_CTRL_R:
    return state & ~(PuglMods)PUGL_MOD_CTRL;
  case PUGL_KEY_ALT_L:
  case PUGL_KEY_ALT_R:
    return state & ~(PuglMods)PUGL_MOD_ALT;
  case PUGL_KEY_SUPER_L:
  case PUGL_KEY_SUPER_R:
    return state & ~(PuglMods)PUGL_MOD_SUPER;
  case PUGL_KEY_NUM_LOCK:
    return state & ~(PuglMods)PUGL_MOD_NUM_LOCK;
  case PUGL_KEY_SCROLL_LOCK:
    return state & ~(PuglMods)PUGL_MOD_SCROLL_LOCK;
  case PUGL_KEY_CAPS_LOCK:
    return state & ~(PuglMods)PUGL_MOD_CAPS_LOCK;
  default:
    break;
  }

  return state;
}

PuglStatus
puglPreRealize(PuglView* const view)
{
  // Ensure that a backend with at least a configure method has been set
  if (!view->backend || !view->backend->configure) {
    return PUGL_BAD_BACKEND;
  }

  // Ensure that the view has an event handler
  if (!view->eventFunc) {
    return PUGL_BAD_CONFIGURATION;
  }

  // Ensure that the default size is set to a valid size
  if (!puglIsValidArea(view->sizeHints[PUGL_DEFAULT_SIZE])) {
    return PUGL_BAD_CONFIGURATION;
  }

  return (view->world->state == PUGL_WORLD_EXPOSING) ? PUGL_BAD_CALL
                                                     : PUGL_SUCCESS;
}

PuglStatus
puglDispatchSimpleEvent(PuglView* view, const PuglEventType type)
{
  assert(type == PUGL_REALIZE || type == PUGL_UNREALIZE ||
         type == PUGL_UPDATE || type == PUGL_CLOSE || type == PUGL_LOOP_ENTER ||
         type == PUGL_LOOP_LEAVE);

  const PuglEvent event = {{type, 0U}};
  return puglDispatchEvent(view, &event);
}

PuglStatus
puglDispatchEvent(PuglView* view, const PuglEvent* event)
{
  PuglStatus st0 = PUGL_SUCCESS;
  PuglStatus st1 = PUGL_SUCCESS;

  switch (event->type) {
  case PUGL_NOTHING:
    break;

  case PUGL_REALIZE:
    assert(view->stage == PUGL_VIEW_STAGE_ALLOCATED);
    if (!(st0 = view->backend->enter(view, NULL))) {
      st0 = view->eventFunc(view, event);
      st1 = view->backend->leave(view, NULL);
    }
    view->stage = PUGL_VIEW_STAGE_REALIZED;
    break;

  case PUGL_UNREALIZE:
    assert(view->stage >= PUGL_VIEW_STAGE_REALIZED);
    if (!(st0 = view->backend->enter(view, NULL))) {
      st0 = view->eventFunc(view, event);
      st1 = view->backend->leave(view, NULL);
    }
    view->stage = PUGL_VIEW_STAGE_ALLOCATED;
    break;

  case PUGL_CONFIGURE:
    st0                 = view->eventFunc(view, event);
    view->lastConfigure = event->configure;
    if (view->stage == PUGL_VIEW_STAGE_REALIZED) {
      view->stage = PUGL_VIEW_STAGE_CONFIGURED;
    }
    break;

  case PUGL_EXPOSE:
    assert(view->stage == PUGL_VIEW_STAGE_CONFIGURED);
    if (!(st0 = view->backend->enter(view, &event->expose))) {
      const PuglWorldState old_state = view->world->state;

      view->world->state = PUGL_WORLD_EXPOSING;
      st0                = view->eventFunc(view, event);
      view->world->state = old_state;
      st1                = view->backend->leave(view, &event->expose);
    }
    break;

  default:
    st0 = view->eventFunc(view, event);
  }

  return st0 ? st0 : st1;
}
