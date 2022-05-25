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

#include "implementation.h"

#include "types.h"

#include "pugl/pugl.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

const char*
puglStrerror(const PuglStatus status)
{
  // clang-format off
  switch (status) {
  case PUGL_SUCCESS:               return "Success";
  case PUGL_FAILURE:               return "Non-fatal failure";
  case PUGL_UNKNOWN_ERROR:         return "Unknown system error";
  case PUGL_BAD_BACKEND:           return "Invalid or missing backend";
  case PUGL_BAD_CONFIGURATION:     return "Invalid view configuration";
  case PUGL_BAD_PARAMETER:         return "Invalid parameter";
  case PUGL_BACKEND_FAILED:        return "Backend initialisation failed";
  case PUGL_REGISTRATION_FAILED:   return "Class registration failed";
  case PUGL_REALIZE_FAILED:        return "View creation failed";
  case PUGL_SET_FORMAT_FAILED:     return "Failed to set pixel format";
  case PUGL_CREATE_CONTEXT_FAILED: return "Failed to create drawing context";
  case PUGL_UNSUPPORTED_TYPE:      return "Unsupported data type";
  }
  // clang-format on

  return "Unknown error";
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

void
puglSetBlob(PuglBlob* const dest, const void* const data, const size_t len)
{
  if (data) {
    dest->len  = len;
    dest->data = realloc(dest->data, len + 1);
    memcpy(dest->data, data, len);
    ((char*)dest->data)[len] = 0;
  } else {
    dest->len  = 0;
    dest->data = NULL;
  }
}

static void
puglSetDefaultHints(PuglHints hints)
{
  hints[PUGL_USE_COMPAT_PROFILE]    = PUGL_TRUE;
  hints[PUGL_CONTEXT_VERSION_MAJOR] = 2;
  hints[PUGL_CONTEXT_VERSION_MINOR] = 0;
  hints[PUGL_RED_BITS]              = 8;
  hints[PUGL_GREEN_BITS]            = 8;
  hints[PUGL_BLUE_BITS]             = 8;
  hints[PUGL_ALPHA_BITS]            = 8;
  hints[PUGL_DEPTH_BITS]            = 0;
  hints[PUGL_STENCIL_BITS]          = 0;
  hints[PUGL_SAMPLES]               = 0;
  hints[PUGL_DOUBLE_BUFFER]         = PUGL_TRUE;
  hints[PUGL_SWAP_INTERVAL]         = PUGL_DONT_CARE;
  hints[PUGL_RESIZABLE]             = PUGL_FALSE;
  hints[PUGL_IGNORE_KEY_REPEAT]     = PUGL_FALSE;
  hints[PUGL_REFRESH_RATE]          = PUGL_DONT_CARE;
}

PuglWorld*
puglNewWorld(PuglWorldType type, PuglWorldFlags flags)
{
  PuglWorld* world = (PuglWorld*)calloc(1, sizeof(PuglWorld));
  if (!world || !(world->impl = puglInitWorldInternals(type, flags))) {
    free(world);
    return NULL;
  }

  world->startTime = puglGetTime(world);

  puglSetString(&world->className, "Pugl");

  return world;
}

void
puglFreeWorld(PuglWorld* const world)
{
  puglFreeWorldInternals(world);
  free(world->className);
  free(world->views);
  free(world);
}

void
puglSetWorldHandle(PuglWorld* world, PuglWorldHandle handle)
{
  world->handle = handle;
}

PuglWorldHandle
puglGetWorldHandle(PuglWorld* world)
{
  return world->handle;
}

PuglStatus
puglSetClassName(PuglWorld* const world, const char* const name)
{
  puglSetString(&world->className, name);
  return PUGL_SUCCESS;
}

PuglView*
puglNewView(PuglWorld* const world)
{
  PuglView* view = (PuglView*)calloc(1, sizeof(PuglView));
  if (!view || !(view->impl = puglInitViewInternals())) {
    free(view);
    return NULL;
  }

  view->world     = world;
  view->minWidth  = 1;
  view->minHeight = 1;

  puglSetDefaultHints(view->hints);

  // Add to world view list
  ++world->numViews;
  world->views =
    (PuglView**)realloc(world->views, world->numViews * sizeof(PuglView*));

  world->views[world->numViews - 1] = view;

  return view;
}

void
puglFreeView(PuglView* view)
{
  if (view->eventFunc && view->backend) {
    puglDispatchSimpleEvent(view, PUGL_DESTROY);
  }

  // Remove from world view list
  PuglWorld* world = view->world;
  for (size_t i = 0; i < world->numViews; ++i) {
    if (world->views[i] == view) {
      if (i == world->numViews - 1) {
        world->views[i] = NULL;
      } else {
        memmove(world->views + i,
                world->views + i + 1,
                sizeof(PuglView*) * (world->numViews - i - 1));
        world->views[world->numViews - 1] = NULL;
      }
      --world->numViews;
    }
  }

  free(view->title);
  free(view->clipboard.data);
  puglFreeViewInternals(view);
  free(view);
}

PuglWorld*
puglGetWorld(PuglView* view)
{
  return view->world;
}

PuglStatus
puglSetViewHint(PuglView* view, PuglViewHint hint, int value)
{
  if (value == PUGL_DONT_CARE) {
    switch (hint) {
    case PUGL_USE_COMPAT_PROFILE:
    case PUGL_USE_DEBUG_CONTEXT:
    case PUGL_CONTEXT_VERSION_MAJOR:
    case PUGL_CONTEXT_VERSION_MINOR:
    case PUGL_SWAP_INTERVAL:
      return PUGL_BAD_PARAMETER;
    default:
      break;
    }
  }

  if (hint < PUGL_NUM_VIEW_HINTS) {
    view->hints[hint] = value;
    return PUGL_SUCCESS;
  }

  return PUGL_BAD_PARAMETER;
}

int
puglGetViewHint(const PuglView* view, PuglViewHint hint)
{
  return (hint < PUGL_NUM_VIEW_HINTS) ? view->hints[hint] : PUGL_DONT_CARE;
}

PuglStatus
puglSetParentWindow(PuglView* view, PuglNativeView parent)
{
  view->parent = parent;
  return PUGL_SUCCESS;
}

PuglStatus
puglSetBackend(PuglView* view, const PuglBackend* backend)
{
  view->backend = backend;
  return PUGL_SUCCESS;
}

void
puglSetHandle(PuglView* view, PuglHandle handle)
{
  view->handle = handle;
}

PuglHandle
puglGetHandle(PuglView* view)
{
  return view->handle;
}

bool
puglGetVisible(const PuglView* view)
{
  return view->visible;
}

PuglRect
puglGetFrame(const PuglView* view)
{
  return view->frame;
}

void*
puglGetContext(PuglView* view)
{
  return view->backend->getContext(view);
}

#ifndef PUGL_DISABLE_DEPRECATED

PuglStatus
puglPollEvents(PuglWorld* world, double timeout)
{
  return puglUpdate(world, timeout);
}

PuglStatus
puglDispatchEvents(PuglWorld* world)
{
  return puglUpdate(world, 0.0);
}

PuglStatus
puglShowWindow(PuglView* view)
{
  return puglShow(view);
}

PuglStatus
puglHideWindow(PuglView* view)
{
  return puglHide(view);
}

#endif

PuglStatus
puglSetEventFunc(PuglView* view, PuglEventFunc eventFunc)
{
  view->eventFunc = eventFunc;
  return PUGL_SUCCESS;
}

/// Return the code point for buf, or the replacement character on error
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

static inline bool
puglMustConfigure(PuglView* view, const PuglConfigureEvent* configure)
{
  return memcmp(configure, &view->lastConfigure, sizeof(PuglConfigureEvent));
}

void
puglDispatchSimpleEvent(PuglView* view, const PuglEventType type)
{
  assert(type == PUGL_CREATE || type == PUGL_DESTROY || type == PUGL_MAP ||
         type == PUGL_UNMAP || type == PUGL_UPDATE || type == PUGL_CLOSE ||
         type == PUGL_LOOP_ENTER || type == PUGL_LOOP_LEAVE);

  const PuglEvent event = {{type, 0}};
  puglDispatchEvent(view, &event);
}

void
puglConfigure(PuglView* view, const PuglEvent* event)
{
  assert(event->type == PUGL_CONFIGURE);

  view->frame.x      = event->configure.x;
  view->frame.y      = event->configure.y;
  view->frame.width  = event->configure.width;
  view->frame.height = event->configure.height;

  if (puglMustConfigure(view, &event->configure)) {
    view->eventFunc(view, event);
    view->lastConfigure = event->configure;
  }
}

void
puglExpose(PuglView* view, const PuglEvent* event)
{
  if (event->expose.width > 0.0 && event->expose.height > 0.0) {
    view->eventFunc(view, event);
  }
}

void
puglDispatchEvent(PuglView* view, const PuglEvent* event)
{
  switch (event->type) {
  case PUGL_NOTHING:
    break;
  case PUGL_CREATE:
  case PUGL_DESTROY:
    view->backend->enter(view, NULL);
    view->eventFunc(view, event);
    view->backend->leave(view, NULL);
    break;
  case PUGL_CONFIGURE:
    if (puglMustConfigure(view, &event->configure)) {
      view->backend->enter(view, NULL);
      puglConfigure(view, event);
      view->backend->leave(view, NULL);
    }
    break;
  case PUGL_MAP:
    if (!view->visible) {
      view->visible = true;
      view->eventFunc(view, event);
    }
    break;
  case PUGL_UNMAP:
    if (view->visible) {
      view->visible = false;
      view->eventFunc(view, event);
    }
    break;
  case PUGL_EXPOSE:
    view->backend->enter(view, &event->expose);
    puglExpose(view, event);
    view->backend->leave(view, &event->expose);
    break;
  default:
    view->eventFunc(view, event);
  }
}

const void*
puglGetInternalClipboard(const PuglView* const view,
                         const char** const    type,
                         size_t* const         len)
{
  if (len) {
    *len = view->clipboard.len;
  }

  if (type) {
    *type = "text/plain";
  }

  return view->clipboard.data;
}

PuglStatus
puglSetInternalClipboard(PuglView* const   view,
                         const char* const type,
                         const void* const data,
                         const size_t      len)
{
  if (type && !!strcmp(type, "text/plain")) {
    return PUGL_UNSUPPORTED_TYPE;
  }

  puglSetBlob(&view->clipboard, data, len);
  return PUGL_SUCCESS;
}
