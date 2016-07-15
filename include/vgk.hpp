#pragma once

#include <cstdint>
#include <vector>
#include <memory>

// //nanovg typedefs
// typedef struct NVGparams NVGparams;
// typedef struct NVGpath NVGpath;
// typedef struct NVGvertex NVGvertex;
// typedef struct NVGscissor NVGscissor;
// typedef struct NVGpaint NVGpaint;
// typedef struct NVGcolor NVGcolor;
typedef struct NVGcontext NVGcontext;
namespace vpp { class SwapChain; }

namespace vgk
{

///This function can be called to create a new nanovg vulkan context.
NVGcontext* create(const vpp::SwapChain& swapChain, bool antiAlias = true);
void destroy(NVGcontext& ctx);

///Returns the assicated renderer for a nanovg context.
class Renderer;
Renderer* renderer(const NVGcontext& ctx);

}
