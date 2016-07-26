///Include vulkan.h before including this file.

#ifndef VVG_INCLUDE_NANOVG_VK_H
#define VVG_INCLUDE_NANOVG_VK_H

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NVGcontext NVGcontext;

///This function can be called to create a new nanovg vulkan context that will render
///on the given swapChain.
NVGcontext* vvgCreateFromSwapchain(VkDevice device, VkSwapchainKHR swapChain, unsigned int format);

///Creates a nanovg context that will render into the given framebuffer.
NVGcontext* vvgCreateFromFramebuffer(unsigned int width, unsigned int height, VkFramebuffer framebuffer);

///Destroys the given nanovg context.
void vvgDestroy(const NVGcontext* ctx);

#ifdef __cplusplus
} //extern C
#endif

#endif //header guard
