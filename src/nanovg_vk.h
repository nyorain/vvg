#ifndef VVG_INCLUDE_NANOVG_VK_H
#define VVG_INCLUDE_NANOVG_VK_H

#pragma once

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NVGcontext NVGcontext;

/// Description for a vulkan nanovg context.
typedef struct VVGContextDescription {
	VkInstance instance; // the instance to create the context for
	VkPhysicalDevice phDev; // the physical device to create the context for
	VkDevice device; // the device to create the context for. Must match given instance and phDev
	VkQueue queue;  // the queue that should be use for rendering. Must support graphcis ops.
	unsigned int queueFamily; // the queue family of the given queue.
	VkSwapchainKHR swapchain; // the swapchain on which should be rendered.
	VkExtent2D swapchainSize; // the size of the given swapchain
	VkFormat swapchainFormat; // the format of the given swapchain
} VVGContextDescription;

/// This function can be called to create a new nanovg vulkan context that will render
/// on the given swapchain.
NVGcontext* vvgCreate(const VVGContextDescription* description);

/// Destroys the given nanovg context.
void vvgDestroy(const NVGcontext* ctx);

#ifdef __cplusplus
} //extern C
#endif

#endif // header guard

// Copyright © 2016 nyorain
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the “Software”), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
