#include <vgk.hpp>

#include <nanovg/nanovg.h>

#include <vpp/device.hpp>
#include <vpp/swapChain.hpp>
#include <vpp/graphicsPipeline.hpp>
#include <vpp/buffer.hpp>
#include <vpp/provider.hpp>
#include <vpp/defs.hpp>
#include <vpp/image.hpp>
#include <vpp/vulkan/range.hpp>

#include <stdexcept>
#include <cstring>
#include <algorithm>

//implementation TODO:
//-try to use push constants for uniform (check maxPushConstantsSize)

namespace vgk
{

//nanovg implementation
namespace
{

//utility
Renderer& resolve(void* ptr)
{
	if(!ptr) throw std::logic_error("vkg::impl: function called with nullptr");
	return *static_cast<Renderer*>(ptr);
}

bool operator==(const NVGcolor& a, const NVGcolor& b)
{
	return (std::memcmp(&a, &b, sizeof(a)) == 0);
}

bool operator!=(const NVGcolor& a, const NVGcolor& b)
{
	return !(a == b);
}

//impl
int renderCreate(void* uptr)
{
}

int createTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data)
{
	auto& renderer = resolve(uptr);
	return renderer.createTexture(type, w, h, imageFlags, *data);
}
int deleteTexture(void* uptr, int image)
{
	auto& renderer = resolve(uptr);
	return renderer.deleteTexture(image);
}
int updateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data)
{
	auto& renderer = resolve(uptr);
	auto* tex = renderer.texture(image);
	if(!tex) throw std::logic_error("vkg::impl::updateTexture: invalid image");

	//tex->update(data);
}
int getTextureSize(void* uptr, int image, int* w, int* h)
{
	auto& renderer = resolve(uptr);
	auto* tex = renderer.texture(image);
	if(!tex) throw std::logic_error("vkg::impl::updateTexture: invalid image");

	//todo: first check pointers
	*w = tex->width();
	*h = tex->width();
}
void viewport(void* uptr, int width, int height)
{
	auto& renderer = resolve(uptr);
	renderer.viewport(width, height);
}
void cancel(void* uptr)
{
	auto& renderer = resolve(uptr);
	renderer.cancel();
}
void flush(void* uptr)
{
	auto& renderer = resolve(uptr);
	renderer.flush();
}
void fill(void* uptr, NVGpaint* paint, NVGscissor* scissor, float fringe, const float* bounds,
	const NVGpath* paths, int npaths)
{
	auto& renderer = resolve(uptr);
	renderer.fill(*paint, *scissor, fringe, bounds, {paths, std::size_t(npaths)});
}
void stroke(void* uptr, NVGpaint* paint, NVGscissor* scissor, float fringe, float strokeWidth,
	const NVGpath* paths, int npaths)
{
	auto& renderer = resolve(uptr);
	renderer.stroke(*paint, *scissor, fringe, strokeWidth, {paths, std::size_t(npaths)});
}
void triangles(void* uptr, NVGpaint* paint, NVGscissor* scissor, const NVGvertex* verts, int nverts)
{
	auto& renderer = resolve(uptr);
	renderer.triangles(*paint, *scissor, {verts, std::size_t(nverts)});
}
void renderDelete(void* uptr)
{
	auto& renderer = resolve(uptr);
	delete &renderer;
}

const NVGparams nvgContextImpl =
{
	nullptr,
	0,
	renderCreate,
	createTexture,
	deleteTexture,
	updateTexture,
	getTextureSize,
	viewport,
	cancel,
	flush,
	fill,
	stroke,
	triangles,
	renderDelete
};

//renderer builder
struct RenderImpl : public vpp::RendererBuilder
{
	std::vector<vk::ClearValue> clearValues(unsigned int id) override;
	void build(unsigned int id, const vpp::RenderPassInstance& ini) override;
	void frame(unsigned int id) override;

	Renderer* renderer;
	vpp::SwapChainRenderer* swapChainRenderer;
};

void RenderImpl::build(unsigned int id, const vpp::RenderPassInstance& ini)
{
	vk::cmdExecuteCommands(ini.vkCommandBuffer(), {renderer->commandBuffer()});
}

std::vector<vk::ClearValue> RenderImpl::clearValues(unsigned int id)
{
	std::vector<vk::ClearValue> ret(2, vk::ClearValue{});
	ret[0].color = {{0.f, 1.f, 0.f, 0.5f}};
	ret[1].depthStencil = {1.f, 0};
	return ret;
}

void RenderImpl::frame(unsigned int id)
{
	swapChainRenderer->record(id);
}

}

//create function
NVGcontext* create(const vpp::SwapChain& swapChain)
{
	auto renderer = new Renderer(swapChain);

	auto params = nvgContextImpl;
	params.userPtr = renderer;
	params.edgeAntiAlias = 1;

	auto ret = nvgCreateInternal(&params);

	if(!ret) delete renderer;
	return ret;
}

void destroy(NVGcontext& context)
{
	nvgDeleteInternal(&context);
}

//Renderer
struct Renderer::Impl
{
	vpp::Buffer uniformBuffer;
	vpp::Buffer vertexBuffer;

	vk::DescriptorPool descriptorPool;
	vpp::DescriptorSet descriptorSet;

	vpp::GraphicsPipeline pipeline;

	vpp::CommandBuffer cmdBuffer {};
	vpp::SwapChainRenderer renderer {};
	vpp::RenderPass renderPass {};
};

Renderer::Renderer(const vpp::SwapChain& swapChain)
{
	constexpr auto uniformSize = 8 + 4 * 2 + 16 * 2 + 64 * 2; //see fill.frag
	constexpr auto vertexSize = 1024 * 1024; //1MB, just a guess
	auto& dev = swapChain.device();

	impl_ = std::make_unique<Impl>();
	initRenderPass(swapChain);

	//create the uniform buffer
	//host visible since it will be changed every drawing command
	vk::BufferCreateInfo bufInfo;
	bufInfo.usage = vk::BufferUsageBits::uniformBuffer;
	bufInfo.size = uniformSize;
	impl_->uniformBuffer = vpp::Buffer(dev, bufInfo, vk::MemoryPropertyBits::hostVisible);

	//create vertex buffer
	//just guess a size that could be enough for most operations
	//it will be resized(recreated) if needed.
	bufInfo.usage = vk::BufferUsageBits::vertexBuffer;
	bufInfo.size = vertexSize;
	impl_->vertexBuffer = vpp::Buffer(dev, bufInfo, vk::MemoryPropertyBits::hostVisible);

	//create the graphics pipeline
	vpp::GraphicsPipeline::CreateInfo pipelineInfo;
	pipelineInfo.states = {{0.f, 0.f, float(swapChain.size().width), float(swapChain.size().height)}};
	pipelineInfo.renderPass = impl_->renderPass;

	//layouts
	//vertex
	vpp::VertexBufferLayout vertexLayout {{vk::Format::r32g32Sfloat, vk::Format::r32g32Sfloat}};
	pipelineInfo.vertexBufferLayouts = {vertexLayout};

	std::vector<vpp::DescriptorBinding> descriptorBindings {
		{vk::DescriptorType::uniformBuffer, vk::ShaderStageBits::vertex},
		{vk::DescriptorType::uniformBuffer, vk::ShaderStageBits::fragment}
	};

	vpp::DescriptorSetLayout descLayout(dev, descriptorBindings);
	pipelineInfo.descriptorSetLayouts = {descLayout};

	//shader
	pipelineInfo.shader = vpp::ShaderProgram(dev);
	pipelineInfo.shader.stage("bin/shaders/fill.vert.spv", {vk::ShaderStageBits::vertex});

	//the fragment shader has a constant for antialiasing
	std::uint32_t antiAliasing = 1;
	vk::SpecializationMapEntry entry {0, 0, 4};

	vk::SpecializationInfo specInfo;
	specInfo.mapEntryCount = 1;
	specInfo.pMapEntries = &entry;
	specInfo.dataSize = 4;
	specInfo.pData = &antiAliasing;

	pipelineInfo.shader.stage("bin/shaders/fill.frag.spv", {vk::ShaderStageBits::fragment, &specInfo});

	impl_->pipeline = vpp::GraphicsPipeline(dev, pipelineInfo);

	impl_->cmdBuffer = dev.commandProvider().get(0, vk::CommandPoolCreateBits::resetCommandBuffer,
		vk::CommandBufferLevel::secondary);

	//descriptor

	//create the swap chain renderer
	auto impl = std::make_unique<RenderImpl>();
	impl->renderer = this;
	impl->swapChainRenderer = &impl_->renderer;
	vpp::SwapChainRenderer::CreateInfo info {impl_->renderPass, 0,
		{{vpp::ViewableImage::defaultColor2D()}}};
	impl_->renderer = vpp::SwapChainRenderer(swapChain, info, std::move(impl));
}

Renderer::~Renderer()
{

}

int Renderer::createTexture(int type, int w, int h, int flags, const std::uint8_t& data)
{
	++texID_;
	textures_.emplace_back(*this, texID_, type, w, h, flags, data);
	return texID_;
}

int Renderer::deleteTexture(int id)
{
	auto it = std::find_if(textures_.begin(), textures_.end(),
		[=](const auto& tex) { return tex.id() == id; });
	//XXX: error handling
	textures_.erase(it);
}

void Renderer::viewport(int width, int height)
{
	//begin the frame
	vk::CommandBufferInheritanceInfo inheritanceInfo;
	inheritanceInfo.renderPass = impl_->renderPass;

	vk::CommandBufferBeginInfo info;
	info.flags = vk::CommandBufferUsageBits::renderPassContinue;
	info.pInheritanceInfo = &inheritanceInfo;

	vk::beginCommandBuffer(impl_->cmdBuffer, info);

	vk::cmdBindPipeline(impl_->cmdBuffer, vk::PipelineBindPoint::graphics,
		impl_->pipeline);
	vk::cmdBindDescriptorSets(impl_->cmdBuffer, vk::PipelineBindPoint::graphics,
		impl_->pipeline.vkPipelineLayout(), 0, {impl_->descriptorSet}, {});
}

void Renderer::cancel()
{
	vk::endCommandBuffer(impl_->cmdBuffer);
	//dont render here
}

void Renderer::flush()
{
	//end the frame
	vk::endCommandBuffer(impl_->cmdBuffer);
	impl_->renderer.renderBlock();
}

void Renderer::fill(const NVGpaint& paint, const NVGscissor& scissor, float fringe, const float* bounds,
	const vpp::Range<NVGpath>& paths)
{

}
void Renderer::stroke(const NVGpaint& paint, const NVGscissor& scissor, float fringe, float strokeWidth,
	const vpp::Range<NVGpath>& paths)
{

}
void Renderer::triangles(const NVGpaint& paint, const NVGscissor& scissor,
	const vpp::Range<NVGvertex>& verts)
{

}

void Renderer::parsePaint(const NVGpaint& paint, const NVGscissor& scissor)
{
	static constexpr auto typeColor = 1;
	static constexpr auto typeGradient = 2;
	static constexpr auto typeTexture = 3;

	vpp::BufferUpdate update(impl_->uniformBuffer);
	auto type = 0u;
	if(paint.image)
	{
		type = typeTexture;
	}
	else if(paint.innerColor == paint.outerColor)
	{
		type = typeColor;
	}
	else
	{
		type = typeTexture;
	}
}

const vpp::CommandBuffer& Renderer::commandBuffer() const
{
	return impl_->cmdBuffer;
}

Texture* Renderer::texture(unsigned int id)
{
	auto it = std::find_if(textures_.begin(), textures_.end(),
		[=](const auto& tex) { return tex.id() == id; });

	if(it == textures_.end()) return nullptr;
	return &(*it);
}

void Renderer::initRenderPass(const vpp::SwapChain& swapChain)
{
	auto& dev = swapChain.device();
	vk::AttachmentDescription attachments[2] {};

	//color from swapchain
	attachments[0].format = swapChain.format();
	attachments[0].samples = vk::SampleCountBits::e1;
	attachments[0].loadOp = vk::AttachmentLoadOp::clear;
	attachments[0].storeOp = vk::AttachmentStoreOp::store;
	attachments[0].stencilLoadOp = vk::AttachmentLoadOp::dontCare;
	attachments[0].stencilStoreOp = vk::AttachmentStoreOp::dontCare;
	attachments[0].initialLayout = vk::ImageLayout::undefined;
	attachments[0].finalLayout = vk::ImageLayout::presentSrcKHR;

	vk::AttachmentReference colorReference;
	colorReference.attachment = 0;
	colorReference.layout = vk::ImageLayout::colorAttachmentOptimal;

	//depth from own depth stencil
	attachments[1].format = vk::Format::d16UnormS8Uint;
	attachments[1].samples = vk::SampleCountBits::e1;
	attachments[1].loadOp = vk::AttachmentLoadOp::clear;
	attachments[1].storeOp = vk::AttachmentStoreOp::store;
	attachments[1].stencilLoadOp = vk::AttachmentLoadOp::dontCare;
	attachments[1].stencilStoreOp = vk::AttachmentStoreOp::dontCare;
	attachments[1].initialLayout = vk::ImageLayout::undefined;
	attachments[1].finalLayout = vk::ImageLayout::undefined;
	// attachments[1].initialLayout = vk::ImageLayout::depthStencilAttachmentOptimal;
	// attachments[1].finalLayout = vk::ImageLayout::depthStencilAttachmentOptimal;

	vk::AttachmentReference depthReference;
	depthReference.attachment = 1;
	depthReference.layout = vk::ImageLayout::depthStencilAttachmentOptimal;

	//only subpass
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::graphics;
	subpass.flags = {};
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depthReference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = nullptr;

	impl_->renderPass = vpp::RenderPass(dev, renderPassInfo);
}

}
