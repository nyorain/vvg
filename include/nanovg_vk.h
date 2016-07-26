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

//XXX To the user of this header: Include vulkan.h before including this file.

#ifndef VVG_INCLUDE_NANOVG_VK_H
#define VVG_INCLUDE_NANOVG_VK_H

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NVGcontext NVGcontext;

///This function can be called to create a new nanovg vulkan context that will render
///on the given swapChain.
NVGcontext* vvgCreateFromSwapchain(VkDevice device, VkSwapchainKHR swapChain, VkFormat format);

///Creates a nanovg context that will render into the given framebuffer.
NVGcontext* vvgCreateFromFramebuffer(unsigned int w, unsigned int h, VkFramebuffer fb);

///Destroys the given nanovg context.
void vvgDestroy(const NVGcontext* ctx);

#ifdef __cplusplus
} //extern C
#endif

#endif //header guard
