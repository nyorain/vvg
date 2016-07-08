#pragma once

#include <cstdint>
#include <vector>
#include <memory>

//nanovg typedefs
typedef struct NVGparams NVGparams;
typedef struct NVGpath NVGpath;
typedef struct NVGvertex NVGvertex;
typedef struct NVGscissor NVGscissor;
typedef struct NVGpaint NVGpaint;
typedef struct NVGcolor NVGcolor;
typedef struct NVGcontext NVGcontext;

//vpp typedefs
namespace vk { namespace range { template <typename T> class Range; }}
namespace vpp
{
	class Device;
	class SwapChain;
	class SwapChainRenderer;
	class CommandBuffer;

	using namespace vk::range;
}

namespace vgk
{

///This function can be called to create a new nanovg vulkan context.
NVGcontext* create(const vpp::SwapChain& swapChain);
void destroy(NVGcontext& ctx);

///Returns the assicated renderer for a nanovg context.
class Renderer;
Renderer* renderer(const NVGcontext& ctx);

class Texture
{
public:
	Texture(Renderer& renderer, unsigned int xid, unsigned int format, unsigned int w,
		unsigned int h, int flags, const std::uint8_t& data);
	~Texture();

	unsigned int id() const { return id_; }
	unsigned int width() const { return width_; }
	unsigned int height() const { return height_; }

protected:
	unsigned int id_;
	unsigned int width_;
	unsigned int height_;
};

class Renderer
{
public:
	Renderer(const vpp::SwapChain& swapChain);
	~Renderer();

	int createTexture(int type, int w, int h, int flags, const std::uint8_t& data);
	int deleteTexture(int id);

	void viewport(int width, int height);
	void cancel();
	void flush();
	void fill(const NVGpaint& paint, const NVGscissor& scissor, float fringe, const float* bounds,
		const vpp::Range<NVGpath>& paths);
	void stroke(const NVGpaint& paint, const NVGscissor& scissor, float fringe, float strokeWidth,
		const vpp::Range<NVGpath>& paths);
	void triangles(const NVGpaint& paint, const NVGscissor& scissor,
		const vpp::Range<NVGvertex>& verts);

	Texture* texture(unsigned int id);
	void parsePaint(const NVGpaint& paint, const NVGscissor& scissor);
	void initRenderPass(const vpp::SwapChain& swapChain);

	const vpp::CommandBuffer& commandBuffer() const;

protected:
	unsigned int texID_ = 0;
	std::vector<Texture> textures_;

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

}
