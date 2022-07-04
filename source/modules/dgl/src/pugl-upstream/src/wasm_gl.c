// Copyright 2012-2022 David Robillard <d@drobilla.net>
// Copyright 2021-2022 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

// #include "attributes.h"
#include "stub.h"
// #include "types.h"
#include "wasm.h"

// #include "pugl/gl.h"
#include "pugl/pugl.h"

#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>

typedef struct {
  EGLDisplay display;
  EGLConfig config;
  EGLContext context;
  EGLSurface surface;
} PuglWasmGlSurface;

static EGLint
puglWasmGlHintValue(const int value)
{
  return value == PUGL_DONT_CARE ? EGL_DONT_CARE : value;
}

static int
puglWasmGlGetAttrib(const EGLDisplay display,
                    const EGLConfig  config,
                    const EGLint     attrib)
{
  EGLint value = 0;
  eglGetConfigAttrib(display, config, attrib, &value);
  return value;
}

static PuglStatus
puglWasmGlConfigure(PuglView* view)
{
  PuglInternals* const impl    = view->impl;
//   const int            screen  = impl->screen;
//   Display* const       display = view->world->impl->display;

  printf("TODO: %s %d | start\n", __func__, __LINE__);

  const EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  if (display == EGL_NO_DISPLAY) {
    printf("eglGetDisplay Failed\n");
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  int major, minor;
  if (eglInitialize(display, &major, &minor) != EGL_TRUE) {
    printf("eglInitialize Failed\n");
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  EGLConfig config;
  int numConfigs;

  if (eglGetConfigs(display, &config, 1, &numConfigs) != EGL_TRUE || numConfigs != 1) {
    printf("eglGetConfigs Failed\n");
    eglTerminate(display);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // clang-format off
  const EGLint attrs[] = {
//     GLX_X_RENDERABLE,  True,
//     GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
//     GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
//     GLX_RENDER_TYPE,   GLX_RGBA_BIT,

//     GLX_DOUBLEBUFFER,  puglX11GlHintValue(view->hints[PUGL_DOUBLE_BUFFER]),

    EGL_SAMPLES,      puglWasmGlHintValue(view->hints[PUGL_SAMPLES]),
    EGL_RED_SIZE,     puglWasmGlHintValue(view->hints[PUGL_RED_BITS]),
    EGL_GREEN_SIZE,   puglWasmGlHintValue(view->hints[PUGL_GREEN_BITS]),
    EGL_BLUE_SIZE,    puglWasmGlHintValue(view->hints[PUGL_BLUE_BITS]),
    EGL_ALPHA_SIZE,   puglWasmGlHintValue(view->hints[PUGL_ALPHA_BITS]),
    EGL_DEPTH_SIZE,   puglWasmGlHintValue(view->hints[PUGL_DEPTH_BITS]),
    EGL_STENCIL_SIZE, puglWasmGlHintValue(view->hints[PUGL_STENCIL_BITS]),
    EGL_NONE
  };
  // clang-format on

  if (eglChooseConfig(display, attrs, &config, 1, &numConfigs) != EGL_TRUE || numConfigs != 1) {
    printf("eglChooseConfig Failed\n");
    eglTerminate(display);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  PuglWasmGlSurface* const surface =
    (PuglWasmGlSurface*)calloc(1, sizeof(PuglWasmGlSurface));
  impl->surface = surface;

  surface->display = display;
  surface->config = config;
  surface->context = EGL_NO_SURFACE;
  surface->surface = EGL_NO_CONTEXT;

  view->hints[PUGL_RED_BITS] =
    puglWasmGlGetAttrib(display, config, EGL_RED_SIZE);
  view->hints[PUGL_GREEN_BITS] =
    puglWasmGlGetAttrib(display, config, EGL_GREEN_SIZE);
  view->hints[PUGL_BLUE_BITS] =
    puglWasmGlGetAttrib(display, config, EGL_BLUE_SIZE);
  view->hints[PUGL_ALPHA_BITS] =
    puglWasmGlGetAttrib(display, config, EGL_ALPHA_SIZE);
  view->hints[PUGL_DEPTH_BITS] =
    puglWasmGlGetAttrib(display, config, EGL_DEPTH_SIZE);
  view->hints[PUGL_STENCIL_BITS] =
    puglWasmGlGetAttrib(display, config, EGL_STENCIL_SIZE);
  view->hints[PUGL_SAMPLES] = puglWasmGlGetAttrib(display, config, EGL_SAMPLES);

  // always enabled for EGL
  view->hints[PUGL_DOUBLE_BUFFER] = 1;

  printf("TODO: %s %d | ok\n", __func__, __LINE__);

  return PUGL_SUCCESS;
}

PUGL_WARN_UNUSED_RESULT
static PuglStatus
puglWasmGlEnter(PuglView* view, const PuglExposeEvent* PUGL_UNUSED(expose))
{
//   printf("DONE: %s %d\n", __func__, __LINE__);
  PuglWasmGlSurface* const surface = (PuglWasmGlSurface*)view->impl->surface;
  if (!surface || !surface->context || !surface->surface) {
    return PUGL_FAILURE;
  }

  return eglMakeCurrent(surface->display, surface->surface, surface->surface, surface->context) ? PUGL_SUCCESS : PUGL_FAILURE;
}

PUGL_WARN_UNUSED_RESULT
static PuglStatus
puglWasmGlLeave(PuglView* view, const PuglExposeEvent* expose)
{
//   printf("DONE: %s %d\n", __func__, __LINE__);
  PuglWasmGlSurface* const surface = (PuglWasmGlSurface*)view->impl->surface;

  if (expose) { // note: swap buffers always enabled for EGL
    eglSwapBuffers(surface->display, surface->surface);
  }

  return eglMakeCurrent(surface->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) ? PUGL_SUCCESS : PUGL_FAILURE;
}

static PuglStatus
puglWasmGlCreate(PuglView* view)
{
  printf("TODO: %s %d | start\n", __func__, __LINE__);
  PuglWasmGlSurface* const surface = (PuglWasmGlSurface*)view->impl->surface;
  const EGLDisplay display = surface->display;
  const EGLConfig  config  = surface->config;

  /*
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
    */

  const EGLint attrs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  surface->context = eglCreateContext(display, config, EGL_NO_CONTEXT, attrs);

  if (surface->context == EGL_NO_CONTEXT) {
    printf("eglCreateContext Failed\n");
    return PUGL_CREATE_CONTEXT_FAILED;
  }

#if 0
  eglMakeCurrent(surface->display, surface->surface, surface->surface, surface->context);

  printf("GL_VENDOR=%s\n", glGetString(GL_VENDOR));
  printf("GL_RENDERER=%s\n", glGetString(GL_RENDERER));
  printf("GL_VERSION=%s\n", glGetString(GL_VERSION));

  eglMakeCurrent(surface->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
#endif

  surface->surface = eglCreateWindowSurface(display, config, 0, NULL);

  if (surface->surface == EGL_NO_SURFACE) {
    printf("eglCreateWindowSurface Failed\n");
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  printf("TODO: %s %d | ok\n", __func__, __LINE__);

  return PUGL_SUCCESS;
}

static void
puglWasmGlDestroy(PuglView* view)
{
  printf("DONE: %s %d\n", __func__, __LINE__);
  PuglWasmGlSurface* surface = (PuglWasmGlSurface*)view->impl->surface;
  if (surface) {
    const EGLDisplay display = surface->display;
    if (surface->surface != EGL_NO_SURFACE)
      eglDestroySurface(display, surface->surface);
    if (surface->context != EGL_NO_CONTEXT)
      eglDestroyContext(display, surface->context);
    eglTerminate(display);
    free(surface);
    view->impl->surface = NULL;
  }
}

const PuglBackend*
puglGlBackend(void)
{
  printf("DONE: %s %d\n", __func__, __LINE__);
  static const PuglBackend backend = {puglWasmGlConfigure,
                                      puglWasmGlCreate,
                                      puglWasmGlDestroy,
                                      puglWasmGlEnter,
                                      puglWasmGlLeave,
                                      puglStubGetContext};

  return &backend;
}
