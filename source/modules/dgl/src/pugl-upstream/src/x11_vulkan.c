/*
  Copyright 2012-2021 David Robillard <d@drobilla.net>

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

#define VK_NO_PROTOTYPES 1

#include "stub.h"
#include "types.h"
#include "x11.h"

#include "pugl/pugl.h"
#include "pugl/vulkan.h"

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_xlib.h>

#include <dlfcn.h>

#include <stdint.h>
#include <stdlib.h>

struct PuglVulkanLoaderImpl {
  void*                     libvulkan;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
  PFN_vkGetDeviceProcAddr   vkGetDeviceProcAddr;
};

PuglVulkanLoader*
puglNewVulkanLoader(PuglWorld* PUGL_UNUSED(world))
{
  PuglVulkanLoader* const loader =
    (PuglVulkanLoader*)calloc(1, sizeof(PuglVulkanLoader));

  if (!loader || !(loader->libvulkan = dlopen("libvulkan.so", RTLD_LAZY))) {
    free(loader);
    return NULL;
  }

  loader->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(
    loader->libvulkan, "vkGetInstanceProcAddr");

  loader->vkGetDeviceProcAddr =
    (PFN_vkGetDeviceProcAddr)dlsym(loader->libvulkan, "vkGetDeviceProcAddr");

  return loader;
}

void
puglFreeVulkanLoader(PuglVulkanLoader* loader)
{
  if (loader) {
    dlclose(loader->libvulkan);
    free(loader);
  }
}

PFN_vkGetInstanceProcAddr
puglGetInstanceProcAddrFunc(const PuglVulkanLoader* loader)
{
  return loader->vkGetInstanceProcAddr;
}

PFN_vkGetDeviceProcAddr
puglGetDeviceProcAddrFunc(const PuglVulkanLoader* loader)
{
  return loader->vkGetDeviceProcAddr;
}

const PuglBackend*
puglVulkanBackend(void)
{
  static const PuglBackend backend = {puglX11Configure,
                                      puglStubCreate,
                                      puglStubDestroy,
                                      puglStubEnter,
                                      puglStubLeave,
                                      puglStubGetContext};

  return &backend;
}

const char* const*
puglGetInstanceExtensions(uint32_t* const count)
{
  static const char* const extensions[] = {"VK_KHR_surface",
                                           "VK_KHR_xlib_surface"};

  *count = 2;
  return extensions;
}

VkResult
puglCreateSurface(PFN_vkGetInstanceProcAddr          vkGetInstanceProcAddr,
                  PuglView* const                    view,
                  VkInstance                         instance,
                  const VkAllocationCallbacks* const allocator,
                  VkSurfaceKHR* const                surface)
{
  PuglInternals* const impl       = view->impl;
  PuglWorldInternals*  world_impl = view->world->impl;

  PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR =
    (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(instance,
                                                      "vkCreateXlibSurfaceKHR");

  const VkXlibSurfaceCreateInfoKHR info = {
    VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
    NULL,
    0,
    world_impl->display,
    impl->win,
  };

  return vkCreateXlibSurfaceKHR(instance, &info, allocator, surface);
}
