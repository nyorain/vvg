//C++14 header defining the interface to create a nanovg vulkan backend.
//License at the bottom of this file.

#ifndef VVG_INCLUDE_VVG_HPP
#define VVG_INCLUDE_VVG_HPP

#pragma once

#include <vpp/fwd.hpp>
#include <vpp/device.hpp>
#include <vpp/renderer.hpp>
#include <vpp/buffer.hpp>
#include <vpp/pipeline.hpp>
#include <vpp/descriptor.hpp>

typedef struct NVGcontext NVGcontext;
typedef struct NVGvertex NVGvertex;
typedef struct NVGpaint NVGpaint;
typedef struct NVGpath NVGpath;
typedef struct NVGscissor NVGscissor;

///Vulkan Vector Graphics
namespace vvg
{

struct DrawData;

//TODO: make work async, e.g. let texture store a work pointer and only finish it when used.
///Represents a vulkan texture.
///Can be retrieved from the nanovg texture handle using the associated renderer.
class Texture : public vpp::ResourceReference<Texture>
{
public:
	Texture(const vpp::Device& dev, unsigned int xid, const vk::Extent2D& size,
		vk::Format format, const std::uint8_t* data = nullptr);
	~Texture() = default;

	Texture(Texture&& other) noexcept = default;
	Texture& operator=(Texture&& other) noexcept = default;

	///Updates the texture data at the given position and the given size.
	void update(const vk::Offset2D& offset, const vk::Extent2D& size, const std::uint8_t& data);

	unsigned int id() const { return id_; }
	unsigned int width() const { return width_; }
	unsigned int height() const { return height_; }
	vk::Format format() const { return format_; }
	const vpp::ViewableImage& viewableImage() const { return viewableImage_; }

	const auto& resourceRef() const { return viewableImage_; }

protected:
	vpp::ViewableImage viewableImage_;
	vk::Format format_;
	unsigned int id_;
	unsigned int width_;
	unsigned int height_;
};

//TODO: how to handle swapChain resizes?
///The Renderer class implements the nanovg backend for vulkan using the vpp library.
///It can be used to gain more control over the rendering e.g. to just record the required
///commands to a given command buffer instead of executing them.
///It can also be used to retrieve some implementation handles such as buffers, pools and
///pipelines for custom use.
///The class works (like the gl nanovg implementation) in an delayed manner, i.e.
///it just stores all draw calls and only renders them once finish is called.
///It can either render on a vulkan SwapChain for which it uses the SwapChainRenderer class
///or directly on a framebuffer, then it uses a plain CommandBuffer.
class Renderer : public vpp::ResourceReference<Renderer>
{
public:
	Renderer(const vpp::SwapChain& swapChain);
	Renderer(const vpp::Framebuffer& framebuffer);
	~Renderer();

	///Returns the texture with the given id.
	const Texture* texture(unsigned int id) const;
	Texture* texture(unsigned int id);

	///Fills the given paths with the given paint.
	void fill(const NVGpaint& paint, const NVGscissor& scissor, float fringe, const float* bounds,
		const vpp::Range<NVGpath>& paths);

	///Stokres the given paths with the given paint.
	void stroke(const NVGpaint& paint, const NVGscissor& scissor, float fringe, float strokeWidth,
		const vpp::Range<NVGpath>& paths);

	///Renders the given vertices as triangle lists with the given paint.
	void triangles(const NVGpaint& paint, const NVGscissor& scissor,
		const vpp::Range<NVGvertex>& verts);

	///Start a new frame. Sets the viewport parameters.
	///Effectively resets all stored draw commands.
	///Will invalidate all commandBuffers that were recorded before.
	void start(unsigned int width, unsigned int height);

	///Cancel the current frame.
	void cancel();

	///Flushs the current frame, i.e. renders it on the render target.
	///This call will block until the device has finished its commands.
	void flush();

	///Records all given draw commands since the last start frame call to the given
	///command buffer. Note that the caller must assure that the commandBuffer is in a valid state
	///for this Renderer to record its commands (i.e. recording state, matching renderPass).
	///All commandBuffers will remain valid until the next draw (fill/stroie/triangles) call
	///or until start is called.
	void record(vk::CommandBuffer cmdBuffer);

	///Creates a texture for the given parameters and returns its id.
	unsigned int createTexture(vk::Format format, unsigned int width, unsigned int height,
		const std::uint8_t* data = nullptr);

	///Deletes the texture with the given id.
	///If the given id could not be found returns false.
	bool deleteTexture(unsigned int id);

	const vpp::Sampler& sampler() const { return sampler_; }
	const vpp::RenderPass& renderPass() const { return renderPass_; }
	const vpp::Buffer& uniformBuffer() const { return uniformBuffer_; }
	const vpp::Buffer& vertexBuffer() const { return vertexBuffer_; }
	const vpp::DescriptorPool& descriptorPool() const { return descriptorPool_; }
	const vpp::DescriptorSetLayout& descriptorLayout() const { return descriptorLayout_; }
	const vpp::PipelineLayout& pipelineLayout() const { return pipelineLayout_; }

	const vpp::SwapChain* swapChain() const { return swapChain_; }
	const vpp::SwapChainRenderer& renderer() const { return renderer_; }

	const vpp::Framebuffer* framebuffer() const { return framebuffer_; }
	const vpp::CommandBuffer& commandBuffer() const { return commandBuffer_; }

	const auto& resourceRef() const { return renderPass_; } //renderPass since always valid

protected:
	void init();
	void initRenderPass(const vpp::Device& dev, vk::Format attachment);

	DrawData& parsePaint(const NVGpaint& paint, const NVGscissor& scissor, float fringe,
		float strokeWidth);

protected:
	const vpp::SwapChain* swapChain_ = nullptr; //if rendering on swapChain
	vpp::SwapChainRenderer renderer_; //used if rendering on swapChain

	const vpp::Framebuffer* framebuffer_ = nullptr; //if rendering into framebuffer
	vpp::CommandBuffer commandBuffer_; //commandBuffer to submit if rendering into fb

	unsigned int texID_ = 0; //the currently highest texture id
	std::vector<Texture> textures_;

	vpp::Buffer uniformBuffer_;
	vpp::Buffer vertexBuffer_;

	std::vector<DrawData> drawDatas_;
	std::vector<NVGvertex> vertices_;

	unsigned int width_ {};
	unsigned int height_ {};

	vpp::Sampler sampler_;
	vpp::RenderPass renderPass_;

	vpp::DescriptorPool descriptorPool_;
	vpp::DescriptorSetLayout descriptorLayout_;
	unsigned int descriptorPoolSize_ {}; //current maximal draw calls count

	vpp::PipelineLayout pipelineLayout_;
	vpp::Pipeline fanPipeline_;
	vpp::Pipeline stripPipeline_;
	vpp::Pipeline listPipeline_;
	unsigned int bound_ = 0;

	//settings
	bool edgeAA_ = true;
};

///Creates the nanovg context for the previoiusly created renderer object.
///Note that this constructor can be useful if one wants to keep a reference to the underlaying
///Renderer object.
NVGcontext* createContext(std::unique_ptr<Renderer> renderer);

///Creates the nanovg context for a given SwapChain.
NVGcontext* createContext(const vpp::SwapChain& swapChain);

///Creates the nanovg context for a given framebuffer.
NVGcontext* createContext(const vpp::Framebuffer& fb);

///Destroys a nanovg context that was created by this library.
///Note that passing a nanovg context that was not created by this library results in undefined
///behaviour.
void destroyContext(const NVGcontext& context);

///Returns the underlaying renderer object from a nanovg context.
///Note that passing a nanovg context that was not created by this library results in undefined
///behaviour.
const Renderer& getRenderer(const NVGcontext& context);
Renderer& getRenderer(NVGcontext& context);

}

#endif //header guard

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
