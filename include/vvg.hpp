#ifndef VVG_INCLUDE_VVG_HPP
#define VVG_INCLUDE_VVG_HPP

#pragma once

#include <vpp/fwd.hpp>

typedef struct NVGcontext NVGcontext;

namespace vvg
{

///The Renderer class implements the nanovg backend for vulkan using the vpp library.
///It can be used to gain more control over the rendering e.g. to just record the required
///commands to a given command buffer instead of executing them.
///It can also be used to retrieve some implementation handles such as buffers, pools and
///pipelines for custom use.
class Renderer
{

};

NVGcontext* createContext(const vpp::SwapChain& dev);
NVGcontext* createContext(const vpp::Framebuffer& fb);
void destoryContext(const NVGcontext& context);

}

#endif //header guard
