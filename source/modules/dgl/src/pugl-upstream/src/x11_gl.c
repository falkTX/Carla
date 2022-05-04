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

#include "stub.h"
#include "types.h"
#include "x11.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  GLXFBConfig fb_config;
  GLXContext  ctx;
} PuglX11GlSurface;

static int
puglX11GlHintValue(const int value)
{
  return value == PUGL_DONT_CARE ? (int)GLX_DONT_CARE : value;
}

static int
puglX11GlGetAttrib(Display* const display,
                   GLXFBConfig    fb_config,
                   const int      attrib)
{
  int value = 0;
  glXGetFBConfigAttrib(display, fb_config, attrib, &value);
  return value;
}

static PuglStatus
puglX11GlConfigure(PuglView* view)
{
  PuglInternals* const impl    = view->impl;
  const int            screen  = impl->screen;
  Display* const       display = impl->display;

  PuglX11GlSurface* const surface =
    (PuglX11GlSurface*)calloc(1, sizeof(PuglX11GlSurface));
  impl->surface = surface;

  // clang-format off
  const int attrs[] = {
#ifdef DGL_USE_RGBA
    GLX_RGBA,
#endif
    GLX_X_RENDERABLE,  True,
    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_SAMPLES,       puglX11GlHintValue(view->hints[PUGL_SAMPLES]),
    GLX_RED_SIZE,      puglX11GlHintValue(view->hints[PUGL_RED_BITS]),
    GLX_GREEN_SIZE,    puglX11GlHintValue(view->hints[PUGL_GREEN_BITS]),
    GLX_BLUE_SIZE,     puglX11GlHintValue(view->hints[PUGL_BLUE_BITS]),
    GLX_ALPHA_SIZE,    puglX11GlHintValue(view->hints[PUGL_ALPHA_BITS]),
    GLX_DEPTH_SIZE,    puglX11GlHintValue(view->hints[PUGL_DEPTH_BITS]),
    GLX_STENCIL_SIZE,  puglX11GlHintValue(view->hints[PUGL_STENCIL_BITS]),
    GLX_DOUBLEBUFFER,  puglX11GlHintValue(view->hints[PUGL_DOUBLE_BUFFER]),
    None
  };
  // clang-format on

  int          n_fbc = 0;
  GLXFBConfig* fbc   = glXChooseFBConfig(display, screen, attrs, &n_fbc);
  if (n_fbc <= 0) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  surface->fb_config = fbc[0];
  impl->vi           = glXGetVisualFromFBConfig(impl->display, fbc[0]);

  view->hints[PUGL_RED_BITS] =
    puglX11GlGetAttrib(display, fbc[0], GLX_RED_SIZE);
  view->hints[PUGL_GREEN_BITS] =
    puglX11GlGetAttrib(display, fbc[0], GLX_GREEN_SIZE);
  view->hints[PUGL_BLUE_BITS] =
    puglX11GlGetAttrib(display, fbc[0], GLX_BLUE_SIZE);
  view->hints[PUGL_ALPHA_BITS] =
    puglX11GlGetAttrib(display, fbc[0], GLX_ALPHA_SIZE);
  view->hints[PUGL_DEPTH_BITS] =
    puglX11GlGetAttrib(display, fbc[0], GLX_DEPTH_SIZE);
  view->hints[PUGL_STENCIL_BITS] =
    puglX11GlGetAttrib(display, fbc[0], GLX_STENCIL_SIZE);
  view->hints[PUGL_SAMPLES] = puglX11GlGetAttrib(display, fbc[0], GLX_SAMPLES);
  view->hints[PUGL_DOUBLE_BUFFER] =
    puglX11GlGetAttrib(display, fbc[0], GLX_DOUBLEBUFFER);

  XFree(fbc);

  return PUGL_SUCCESS;
}

static PuglStatus
puglX11GlEnter(PuglView* view, const PuglExposeEvent* PUGL_UNUSED(expose))
{
  PuglX11GlSurface* surface = (PuglX11GlSurface*)view->impl->surface;
  glXMakeCurrent(view->impl->display, view->impl->win, surface->ctx);
  return PUGL_SUCCESS;
}

static PuglStatus
puglX11GlLeave(PuglView* view, const PuglExposeEvent* expose)
{
  if (expose && view->hints[PUGL_DOUBLE_BUFFER]) {
    glXSwapBuffers(view->impl->display, view->impl->win);
  }

  glXMakeCurrent(view->impl->display, None, NULL);

  return PUGL_SUCCESS;
}

static PuglStatus
puglX11GlCreate(PuglView* view)
{
  PuglInternals* const    impl      = view->impl;
  PuglX11GlSurface* const surface   = (PuglX11GlSurface*)impl->surface;
  Display* const          display   = impl->display;
  GLXFBConfig             fb_config = surface->fb_config;

  const int ctx_attrs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB,
    view->hints[PUGL_CONTEXT_VERSION_MAJOR],

    GLX_CONTEXT_MINOR_VERSION_ARB,
    view->hints[PUGL_CONTEXT_VERSION_MINOR],

    GLX_CONTEXT_FLAGS_ARB,
    (view->hints[PUGL_USE_DEBUG_CONTEXT] ? GLX_CONTEXT_DEBUG_BIT_ARB : 0),

    GLX_CONTEXT_PROFILE_MASK_ARB,
    (view->hints[PUGL_USE_COMPAT_PROFILE]
       ? GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
       : GLX_CONTEXT_CORE_PROFILE_BIT_ARB),
    0};

  PFNGLXCREATECONTEXTATTRIBSARBPROC create_context =
    (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress(
      (const uint8_t*)"glXCreateContextAttribsARB");

  PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT =
    (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress(
      (const uint8_t*)"glXSwapIntervalEXT");

  surface->ctx = create_context(display, fb_config, 0, True, ctx_attrs);
  if (!surface->ctx) {
    surface->ctx =
      glXCreateNewContext(display, fb_config, GLX_RGBA_TYPE, 0, True);
  }

  if (!surface->ctx) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  const int swapInterval = view->hints[PUGL_SWAP_INTERVAL];
  if (glXSwapIntervalEXT && swapInterval != PUGL_DONT_CARE) {
    puglX11GlEnter(view, NULL);
    glXSwapIntervalEXT(display, impl->win, swapInterval);
    puglX11GlLeave(view, NULL);
  }

  glXGetConfig(impl->display,
               impl->vi,
               GLX_DOUBLEBUFFER,
               &view->hints[PUGL_DOUBLE_BUFFER]);

  glXQueryDrawable(display,
                   impl->win,
                   GLX_SWAP_INTERVAL_EXT,
                   (unsigned int*)&view->hints[PUGL_SWAP_INTERVAL]);

  return PUGL_SUCCESS;
}

static PuglStatus
puglX11GlDestroy(PuglView* view)
{
  PuglX11GlSurface* surface = (PuglX11GlSurface*)view->impl->surface;
  if (surface) {
    glXDestroyContext(view->impl->display, surface->ctx);
    free(surface);
    view->impl->surface = NULL;
  }
  return PUGL_SUCCESS;
}

PuglGlFunc
puglGetProcAddress(const char* name)
{
  return glXGetProcAddress((const uint8_t*)name);
}

PuglStatus
puglEnterContext(PuglView* view)
{
  return view->backend->enter(view, NULL);
}

PuglStatus
puglLeaveContext(PuglView* view)
{
  return view->backend->leave(view, NULL);
}

const PuglBackend*
puglGlBackend(void)
{
  static const PuglBackend backend = {puglX11GlConfigure,
                                      puglX11GlCreate,
                                      puglX11GlDestroy,
                                      puglX11GlEnter,
                                      puglX11GlLeave,
                                      puglStubGetContext};

  return &backend;
}
