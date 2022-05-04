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

#define VK_NO_PROTOTYPES 1

#include "implementation.h"
#include "mac.h"
#include "stub.h"
#include "types.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"
#include "pugl/vulkan.h"

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_macos.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#include <dlfcn.h>

#include <stdint.h>
#include <stdlib.h>

@interface PuglVulkanView : NSView<CALayerDelegate>

@end

@implementation PuglVulkanView {
@public
  PuglView* puglview;
}

- (id)initWithFrame:(NSRect)frame
{
  self = [super initWithFrame:frame];

  if (self) {
    self.wantsLayer                = YES;
    self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawOnSetNeedsDisplay;
  }

  return self;
}

- (CALayer*)makeBackingLayer
{
  CAMetalLayer* layer = [CAMetalLayer layer];
  [layer setDelegate:self];
  return layer;
}

- (void)setFrameSize:(NSSize)newSize
{
  PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];

  [super setFrameSize:newSize];
  [wrapper setReshaped];

  self.layer.frame = self.bounds;
}

- (void)displayLayer:(CALayer*)layer
{
  (void)layer;
  PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];
  [wrapper dispatchExpose:[self bounds]];
}

@end

static PuglStatus
puglMacVulkanCreate(PuglView* view)
{
  PuglInternals*  impl     = view->impl;
  PuglVulkanView* drawView = [PuglVulkanView alloc];
  const NSRect rect = NSMakeRect(0, 0, view->frame.width, view->frame.height);

  drawView->puglview = view;
  [drawView initWithFrame:rect];
  if (view->hints[PUGL_RESIZABLE]) {
    [drawView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  } else {
    [drawView setAutoresizingMask:NSViewNotSizable];
  }

  impl->drawView = drawView;
  return PUGL_SUCCESS;
}

static PuglStatus
puglMacVulkanDestroy(PuglView* view)
{
  PuglVulkanView* const drawView = (PuglVulkanView*)view->impl->drawView;

  [drawView removeFromSuperview];
  [drawView release];

  view->impl->drawView = nil;
  return PUGL_SUCCESS;
}

struct PuglVulkanLoaderImpl {
  void*                     libvulkan;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
  PFN_vkGetDeviceProcAddr   vkGetDeviceProcAddr;
};

PuglVulkanLoader*
puglNewVulkanLoader(PuglWorld* PUGL_UNUSED(world))
{
  PuglVulkanLoader* loader =
    (PuglVulkanLoader*)calloc(1, sizeof(PuglVulkanLoader));
  if (!loader) {
    return NULL;
  }

  if (!(loader->libvulkan = dlopen("libvulkan.dylib", RTLD_LAZY))) {
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
  static const PuglBackend backend = {puglStubConfigure,
                                      puglMacVulkanCreate,
                                      puglMacVulkanDestroy,
                                      puglStubEnter,
                                      puglStubLeave,
                                      puglStubGetContext};

  return &backend;
}

const char* const*
puglGetInstanceExtensions(uint32_t* const count)
{
  static const char* const extensions[] = {"VK_KHR_surface",
                                           "VK_MVK_macos_surface"};

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
  PuglInternals* const impl = view->impl;

  PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK =
    (PFN_vkCreateMacOSSurfaceMVK)vkGetInstanceProcAddr(
      instance, "vkCreateMacOSSurfaceMVK");

  const VkMacOSSurfaceCreateInfoMVK info = {
    VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
    NULL,
    0,
    impl->drawView,
  };

  return vkCreateMacOSSurfaceMVK(instance, &info, allocator, surface);
}
