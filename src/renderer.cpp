#include "vvg.hpp"
#include "nanovg_vk.h"
#include "nanovg.h"

// vpp
#include <vpp/bufferOps.hpp>
#include <vpp/graphicsPipeline.hpp>
#include <vpp/swapChain.hpp>
#include <vpp/framebuffer.hpp>
#include <vpp/vk.hpp>

// stl
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <iterator>

// shader header
#include "shader/headers/fill.frag.h"
#include "shader/headers/fill.vert.h"

namespace vvg
{

//minimal shader types
struct Vec2 { float x,y; };
struct Vec3 { float x,y,z; };
struct Vec4 { float x,y,z,w; };

using Mat2 = float[2][2];
using Mat3 = float[3][3];
using Mat4 = float[4][4];

struct UniformData
{
	Vec2 viewSize;
	std::uint32_t type;
	std::uint32_t texType;
	Vec4 innerColor;
	Vec4 outerColor;
	Mat4 scissorMat;
	Mat4 paintMat;
};

struct Path
{
	std::size_t fillOffset = 0;
	std::size_t fillCount = 0;
	std::size_t strokeOffset = 0;
	std::size_t strokeCount = 0;
};

struct DrawData
{
	vpp::DescriptorSet descriptorSet;
	UniformData uniformData;
	unsigned int texture = 0;

	std::vector<Path> paths;
	std::size_t triangleOffset = 0;
	std::size_t triangleCount = 0;
};

//The RenderBuilder implementation used to render on a swapChain.
struct RenderImpl : public vpp::RendererBuilder
{
	std::vector<vk::ClearValue> clearValues(unsigned int id) override;
	void build(unsigned int id, const vpp::RenderPassInstance& ini) override;
	void frame(unsigned int id) override;

	Renderer* renderer;
	vpp::SwapChainRenderer* swapChainRenderer;
};

}

namespace vpp
{

template<> struct VulkanType<vvg::Vec2> : public VulkanTypeVec2 {};
template<> struct VulkanType<vvg::Vec3> : public VulkanTypeVec3 {};
template<> struct VulkanType<vvg::Vec4> : public VulkanTypeVec4 {};
template<> struct VulkanType<vvg::Mat2> : public VulkanTypeMatrix<2, 2, true> {};
template<> struct VulkanType<vvg::Mat3> : public VulkanTypeMatrix<3, 3, true> {};
template<> struct VulkanType<vvg::Mat4> : public VulkanTypeMatrix<4, 4, true> {};

template<> struct VulkanType<vvg::UniformData>
{
	static constexpr auto type = vpp::ShaderType::structure;
	static constexpr auto members = std::make_tuple(
		&vvg::UniformData::viewSize,
		&vvg::UniformData::type,
		&vvg::UniformData::texType,
		&vvg::UniformData::innerColor,
		&vvg::UniformData::outerColor,
		&vvg::UniformData::scissorMat,
		&vvg::UniformData::paintMat
	);
};

}


namespace vvg
{

//Renderer
Renderer::Renderer(const vpp::SwapChain& swapChain) : swapChain_(&swapChain)
{
	initRenderPass(swapChain.device(), swapChain.format());
	init();

	auto impl = std::make_unique<RenderImpl>();
	impl->renderer = this;
	impl->swapChainRenderer = &renderer_;

	auto attachmentInfo = vpp::ViewableImage::defaultDepth2D();
	attachmentInfo.imgInfo.format = vk::Format::s8Uint;
	attachmentInfo.viewInfo.format = vk::Format::s8Uint;
	attachmentInfo.viewInfo.subresourceRange.aspectMask = vk::ImageAspectBits::stencil;

	vpp::SwapChainRenderer::CreateInfo info {renderPass_, 0, {{attachmentInfo}}};
	renderer_ = {swapChain, info, std::move(impl)};
}


Renderer::Renderer(const vpp::Framebuffer& framebuffer) : framebuffer_(&framebuffer)
{
	//TODO: implement alternative flush function.
	initRenderPass(framebuffer.device(), vk::Format::r8g8b8a8Unorm); ///XXX: format
	init();
}

Renderer::~Renderer()
{

}

void Renderer::init()
{
	//sampler
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::linear;
	samplerInfo.minFilter = vk::Filter::linear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::linear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::clampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::clampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::clampToEdge;
	samplerInfo.mipLodBias = 0;
	samplerInfo.anisotropyEnable = true;
	samplerInfo.maxAnisotropy = 8;
	samplerInfo.compareEnable = false;
	samplerInfo.compareOp = {};
	samplerInfo.minLod = 0;
	samplerInfo.maxLod = 0;
	samplerInfo.borderColor = vk::BorderColor::floatTransparentBlack;
	samplerInfo.unnormalizedCoordinates = false;
	sampler_ = {device(), samplerInfo};

	//descLayout
	auto descriptorBindings  =
	{
		vpp::descriptorBinding(vk::DescriptorType::uniformBuffer,
			vk::ShaderStageBits::vertex | vk::ShaderStageBits::fragment),
		vpp::descriptorBinding(vk::DescriptorType::combinedImageSampler,
			vk::ShaderStageBits::fragment, -1, 1, &sampler_.vkHandle())
	};

	descriptorLayout_ = {device(), descriptorBindings};
	pipelineLayout_ = {device(), {descriptorLayout_}};

	//create the graphics pipeline
	vpp::GraphicsPipelineBuilder builder(device(), renderPass_);
	builder.dynamicStates = {vk::DynamicState::viewport, vk::DynamicState::scissor};
	builder.states.rasterization.cullMode = vk::CullModeBits::none;
	builder.states.inputAssembly.topology = vk::PrimitiveTopology::triangleFan;
	builder.states.blendAttachments[0].blendEnable = true;
	builder.states.blendAttachments[0].colorBlendOp = vk::BlendOp::add;
	builder.states.blendAttachments[0].srcColorBlendFactor = vk::BlendFactor::srcAlpha;
	builder.states.blendAttachments[0].dstColorBlendFactor = vk::BlendFactor::oneMinusSrcAlpha;
	builder.states.blendAttachments[0].srcAlphaBlendFactor = vk::BlendFactor::one;
	builder.states.blendAttachments[0].dstAlphaBlendFactor = vk::BlendFactor::zero;
	builder.states.blendAttachments[0].alphaBlendOp = vk::BlendOp::add;

	vpp::VertexBufferLayout vertexLayout {{vk::Format::r32g32Sfloat, vk::Format::r32g32Sfloat}};
	builder.vertexBufferLayouts = {vertexLayout};
	builder.layout = pipelineLayout_;

	//shader
	//the fragment shader has a constant for antialiasing
	std::uint32_t antiAliasing = edgeAA_;
	vk::SpecializationMapEntry entry {0, 0, 4};

	vk::SpecializationInfo specInfo;
	specInfo.mapEntryCount = 1;
	specInfo.pMapEntries = &entry;
	specInfo.dataSize = 4;
	specInfo.pData = &antiAliasing;

	builder.shader.stage(fill_vert_data, {vk::ShaderStageBits::vertex});
	builder.shader.stage(fill_frag_data, {vk::ShaderStageBits::fragment, &specInfo});

	fanPipeline_ = builder.build();

	builder.states.inputAssembly.topology = vk::PrimitiveTopology::triangleStrip;
	stripPipeline_ = builder.build();

	builder.states.inputAssembly.topology = vk::PrimitiveTopology::triangleList;
	listPipeline_ = builder.build();
}

unsigned int Renderer::createTexture(vk::Format format, unsigned int w, unsigned int h,
	const std::uint8_t* data)
{
	++texID_;
	textures_.emplace_back(device(), texID_, vk::Extent2D{w, h}, format, data);
	return texID_;
}

bool Renderer::deleteTexture(unsigned int id)
{
	auto it = std::find_if(textures_.begin(), textures_.end(),
		[=](const auto& tex) { return tex.id() == id; });

	if(it == textures_.end()) return false;

	textures_.erase(it);
	return true;
}

void Renderer::start(unsigned int width, unsigned int height)
{
	//store (and set) viewport in some way
	width_ = width;
	height_ = height;

	vertices_.clear();
	drawDatas_.clear();
}

void Renderer::cancel()
{
}

void Renderer::flush()
{
	//allocate buffers
	auto uniformSize = sizeof(UniformData) * drawDatas_.size();
	if(uniformBuffer_.size() < uniformSize)
	{
		vk::BufferCreateInfo bufInfo;
		bufInfo.usage = vk::BufferUsageBits::uniformBuffer;
		bufInfo.size = uniformSize;
		uniformBuffer_ = vpp::Buffer(device(), bufInfo, vk::MemoryPropertyBits::hostVisible);
	}

	auto vertexSize = vertices_.size() * sizeof(NVGvertex);
	if(vertexBuffer_.size() < vertexSize)
	{
		vk::BufferCreateInfo bufInfo;
		bufInfo.usage = vk::BufferUsageBits::vertexBuffer;
		bufInfo.size = vertexSize;
		vertexBuffer_ = vpp::Buffer(device(), bufInfo, vk::MemoryPropertyBits::hostVisible);
	}

	//descriptorPool
	if(drawDatas_.size() > descriptorPoolSize_)
	{
		vk::DescriptorPoolSize typeCounts[2];
		typeCounts[0].type = vk::DescriptorType::uniformBuffer;
		typeCounts[0].descriptorCount = drawDatas_.size();

		typeCounts[1].type = vk::DescriptorType::combinedImageSampler;
		typeCounts[1].descriptorCount = drawDatas_.size();

		vk::DescriptorPoolCreateInfo poolInfo;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = typeCounts;
		poolInfo.maxSets = drawDatas_.size();

		descriptorPool_ = {device(), poolInfo};
		descriptorPoolSize_ = drawDatas_.size();
	}
	else if(descriptorPool_)
	{
		vk::resetDescriptorPool(device(), descriptorPool_, {});
	}

	//update
	vpp::BufferUpdate update(uniformBuffer_, vpp::BufferLayout::std140);
	for(auto& data : drawDatas_)
	{
		update.alignUniform();

		auto offset = update.offset();
		update.add(data.uniformData);

		data.descriptorSet = {descriptorLayout_, descriptorPool_};

		vpp::DescriptorSetUpdate descUpdate(data.descriptorSet);
		descUpdate.uniform({{uniformBuffer_, offset, sizeof(UniformData)}});

		if(data.texture != 0)
		{
			auto* tex = texture(data.texture);
			auto layout = vk::ImageLayout::general; //XXX
			descUpdate.imageSampler({{{}, tex->viewableImage().vkImageView(), layout}});
		}
	}

	update.apply()->finish();

	//vertex
	vpp::BufferUpdate vupdate(vertexBuffer_, vpp::BufferLayout::std140);
	vupdate.add(vpp::raw(*vertices_.data(), vertices_.size()));
	vupdate.apply()->finish();

	//render
	renderer_.renderBlock();

	//cleanup
	vertices_.clear();
	drawDatas_.clear();
}

void Renderer::fill(const NVGpaint& paint, const NVGscissor& scissor, float fringe,
	const float* bounds, const vpp::Range<NVGpath>& paths)
{
	auto& drawData = parsePaint(paint, scissor, fringe, fringe);
	drawData.paths.reserve(paths.size());

	for(auto& path : paths)
	{
		drawData.paths.emplace_back();
		drawData.paths.back().fillOffset = vertices_.size();
		drawData.paths.back().fillCount = path.nfill;
		vertices_.insert(vertices_.end(), path.fill, path.fill + path.nfill);

		if(edgeAA_ && path.nstroke > 0)
		{
			drawData.paths.back().strokeOffset = vertices_.size();
			drawData.paths.back().strokeCount = path.nstroke;
			vertices_.insert(vertices_.end(), path.stroke, path.stroke + path.nstroke);
		}
	}
}
void Renderer::stroke(const NVGpaint& paint, const NVGscissor& scissor, float fringe,
	float strokeWidth, const vpp::Range<NVGpath>& paths)
{
	auto& drawData = parsePaint(paint, scissor, fringe, strokeWidth);
	drawData.paths.reserve(paths.size());

	for(auto& path : paths)
	{
		drawData.paths.emplace_back();
		drawData.paths.back().strokeOffset = vertices_.size();
		drawData.paths.back().strokeCount = path.nstroke;
		vertices_.insert(vertices_.end(), path.stroke, path.stroke + path.nstroke);
	}
}
void Renderer::triangles(const NVGpaint& paint, const NVGscissor& scissor,
	const vpp::Range<NVGvertex>& verts)
{
	auto& drawData = parsePaint(paint, scissor, 1.f, 1.f);

	drawData.triangleOffset = vertices_.size();
	drawData.triangleCount = verts.size();
	vertices_.insert(vertices_.end(), verts.begin(), verts.end());
}

DrawData& Renderer::parsePaint(const NVGpaint& paint, const NVGscissor& scissor, float fringe,
	float strokeWidth)
{
	static constexpr auto typeColor = 1;
	static constexpr auto typeGradient = 2;
	static constexpr auto typeTexture = 3;

	static constexpr auto texTypeRGBA = 1;
	static constexpr auto texTypeA = 2;

	//update image
	drawDatas_.emplace_back();

	auto& data = drawDatas_.back();
	data.uniformData.viewSize = {float(width_), float(height_)};

	if(paint.image)
	{
		auto* tex = texture(paint.image);

		auto formatID = (tex->format() == vk::Format::r8g8b8a8Unorm) ? texTypeRGBA : texTypeA;
		data.uniformData.type = typeTexture;
		data.uniformData.texType = formatID;

		data.texture = paint.image;
	}
	else if(std::memcmp(&paint.innerColor, &paint.outerColor, sizeof(paint.innerColor)) == 0)
	{
		data.uniformData.type = typeColor;
		data.uniformData.texType = 0u;
	}
	else
	{
		data.uniformData.type = typeGradient;
		data.uniformData.texType = 0u;
	}

	//colors
	std::memcpy(&data.uniformData.innerColor, &paint.innerColor, sizeof(float) * 4);
	std::memcpy(&data.uniformData.outerColor, &paint.outerColor, sizeof(float) * 4);

	//mats
	float invxform[6];

	//scissor
	float scissorMat[4][4] {0};
	if (scissor.extent[0] < -0.5f || scissor.extent[1] < -0.5f)
	{
		scissorMat[3][0] = 1.0f;
		scissorMat[3][1] = 1.0f;
		scissorMat[3][2] = 1.0f;
		scissorMat[3][3] = 1.0f;
	}
	else
	{
		nvgTransformInverse(invxform, scissor.xform);

		scissorMat[0][0] = invxform[0];
		scissorMat[0][1] = invxform[1];
		scissorMat[1][0] = invxform[2];
		scissorMat[1][1] = invxform[3];
		scissorMat[2][0] = invxform[4];
		scissorMat[2][1] = invxform[5];
		scissorMat[2][2] = 1.0f;

		//extent
		scissorMat[3][0] = scissor.extent[0];
		scissorMat[3][1] = scissor.extent[1];

		//scale
		scissorMat[3][2] = std::sqrt(scissor.xform[0]*scissor.xform[0] + scissor.xform[2]*
			scissor.xform[2]) / fringe;
		scissorMat[3][3] = std::sqrt(scissor.xform[1]*scissor.xform[1] + scissor.xform[3]*
			scissor.xform[3]) / fringe;

		scissorMat[0][3] = paint.radius;
		scissorMat[1][3] = paint.feather;
		scissorMat[2][3] = strokeWidth;
	}

	std::memcpy(&data.uniformData.scissorMat, &scissorMat, sizeof(scissorMat));

	//paint
	float paintMat[4][4] {};
	nvgTransformInverse(invxform, paint.xform);
	paintMat[0][0] = invxform[0];
	paintMat[0][1] = invxform[1];
	paintMat[1][0] = invxform[2];
	paintMat[1][1] = invxform[3];
	paintMat[2][0] = invxform[4];
	paintMat[2][1] = invxform[5];
	paintMat[2][2] = 1.0f;

	//strokeMult
	paintMat[0][3] = (strokeWidth * 0.5f + fringe * 0.5f) / fringe;

	std::memcpy(&data.uniformData.paintMat, &paintMat, sizeof(paintMat));
	return data;
}

const Texture* Renderer::texture(unsigned int id) const
{
	auto it = std::find_if(textures_.begin(), textures_.end(),
		[=](const auto& tex) { return tex.id() == id; });

	if(it == textures_.end()) return nullptr;
	return &(*it);
}

Texture* Renderer::texture(unsigned int id)
{
	auto it = std::find_if(textures_.begin(), textures_.end(),
		[=](const auto& tex) { return tex.id() == id; });

	if(it == textures_.end()) return nullptr;
	return &(*it);
}

void Renderer::record(vk::CommandBuffer cmdBuffer)
{
	int bound = 0;
	vk::cmdBindVertexBuffers(cmdBuffer, 0, {vertexBuffer_}, {{0}});

	for(auto& data : drawDatas_)
	{
		vk::cmdBindDescriptorSets(cmdBuffer, vk::PipelineBindPoint::graphics, pipelineLayout_,
			0, {data.descriptorSet}, {});

		for(auto& path : data.paths)
		{
			if(path.fillCount > 0)
			{
				if(bound != 1)
				{
					vk::cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::graphics, fanPipeline_);
					bound = 1;
				}

				vk::cmdDraw(cmdBuffer, path.fillCount, 1, path.fillOffset, 0);
			}
			if(path.strokeCount > 0)
			{
				if(bound != 2)
				{
					vk::cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::graphics, stripPipeline_);
					bound = 2;
				}

				vk::cmdDraw(cmdBuffer, path.strokeCount, 1, path.strokeOffset, 0);
			}
		}

		if(data.triangleCount > 0)
		{
			if(bound != 3)
			{
				vk::cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::graphics, listPipeline_);
				bound = 3;
			}

			vk::cmdDraw(cmdBuffer, data.triangleCount, 1, data.triangleOffset, 0);
		}
	}
}

void Renderer::initRenderPass(const vpp::Device& dev, vk::Format attachment)
{
	vk::AttachmentDescription attachments[2] {};

	//color from swapchain
	attachments[0].format = attachment;
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

	//stencil attachment
	//will not be used as depth buffer
	attachments[1].format = vk::Format::s8Uint;
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

	renderPass_ = {dev, renderPassInfo};
}


//Texture
Texture::Texture(const vpp::Device& dev, unsigned int xid, const vk::Extent2D& size,
	vk::Format format, const std::uint8_t* data)
		: format_(format), id_(xid), width_(size.width), height_(size.height)
{
	vk::Extent3D extent {width(), height(), 1};

	auto info = vpp::ViewableImage::defaultColor2D();
	info.imgInfo.extent = extent;
	info.imgInfo.initialLayout = vk::ImageLayout::preinitialized;
	info.imgInfo.tiling = vk::ImageTiling::linear;

	info.imgInfo.format = format;
	info.viewInfo.format = format;

	info.imgInfo.usage = vk::ImageUsageBits::sampled;
	info.memoryFlags = vk::MemoryPropertyBits::hostVisible;

	// info.imgInfo.usage = vk::ImageUsageBits::transferDst | vk::ImageUsageBits::sampled;
	viewableImage_ = {dev, info};

	///TODO: remove finish here and check where the bug comes from. probably vpp submit.cpp
	vpp::changeLayout(viewableImage_.image(), vk::ImageLayout::preinitialized,
		vk::ImageLayout::general, vk::ImageAspectBits::color)->finish();

	if(data)
		vpp::fill(viewableImage_.image(), *data, format, vk::ImageLayout::general, extent,
			{vk::ImageAspectBits::color, 0, 0})->finish();
}

void Texture::update(const vk::Offset2D& offset, const vk::Extent2D& extent,
	const std::uint8_t& data)
{
	//TODO: really only update the given offset/extent
	//would need an extra data copy
	//or modify vpp::fill(Image) to also accept non tightly packed data
	//or just fill it manually...
	
	vk::Extent3D iextent {width() - offset.x, height() - offset.y, 1};
	vk::Offset3D ioffset {offset.x, offset.y, 0};

	// this function is buggy atm
	fill(viewableImage_.image(), data, format(), vk::ImageLayout::general, iextent,
		{vk::ImageAspectBits::color, 0, 0})->finish();
}


//RenderImpl
void RenderImpl::build(unsigned int id, const vpp::RenderPassInstance& ini)
{
	renderer->record(ini.vkCommandBuffer());
}

std::vector<vk::ClearValue> RenderImpl::clearValues(unsigned int id)
{
	std::vector<vk::ClearValue> ret(2, vk::ClearValue{});
	ret[0].color = {{0.f, 0.f, 0.f, 1.0f}};
	ret[1].depthStencil = {1.f, 0};
	return ret;
}

void RenderImpl::frame(unsigned int id)
{
	swapChainRenderer->record(id);
}

}


//The NVGcontext backend implementation which just calls the Renderer/Texture member functions
namespace
{

vvg::Renderer& resolve(void* ptr)
{
	return *static_cast<vvg::Renderer*>(ptr);
}

int renderCreate(void* uptr)
{
}

int createTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data)
{
	auto& renderer = resolve(uptr);
	auto format = (type == NVG_TEXTURE_ALPHA) ? vk::Format::r8Unorm : vk::Format::r8g8b8a8Unorm;
	return renderer.createTexture(format, w, h, data);
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
	if(!tex) return 0;

	vk::Extent2D extent {(unsigned int) w, (unsigned int) h};
	vk::Offset2D offset{x, y};
	tex->update(offset, extent, *data);

	return 1;
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
	renderer.start(width, height);
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
	1,
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

}


//implementation of the C++ create api
namespace vvg
{

NVGcontext* createContext(std::unique_ptr<Renderer> renderer)
{
	auto impl = nvgContextImpl;
	auto rendererPtr = renderer.get();
	impl.userPtr = renderer.release();
	auto ret = nvgCreateInternal(&impl);
	if(!ret) delete rendererPtr;
	return ret;
}

NVGcontext* createContext(const vpp::SwapChain& swapChain)
{
	return createContext(std::make_unique<Renderer>(swapChain));
}

NVGcontext* createContext(const vpp::Framebuffer& framebuffer)
{
	return createContext(std::make_unique<Renderer>(framebuffer));
}

void destroyContext(const NVGcontext& context)
{
	auto ctx = const_cast<NVGcontext*>(&context);
	nvgDeleteInternal(ctx);
}

const Renderer& getRenderer(const NVGcontext& context)
{
	auto ctx = const_cast<NVGcontext*>(&context);
	return resolve(nvgInternalParams(ctx)->userPtr);
}
Renderer& getRenderer(NVGcontext& context)
{
	return resolve(nvgInternalParams(&context)->userPtr);
}

}

//implementation of the C api
NVGcontext* vvgCreateFromSwapchain(VkDevice device, VkSwapchainKHR swapChain, VkFormat format)
{
	// vpp::Device device(instance, physicalDevice, device);
	// auto renderer = vvg::make_unique<vvg::Renderer>(device, swapChain);
	// return vvg::createContext(std::move(renderer));
}

NVGcontext* vvgCreateFromFramebuffer(unsigned int w, unsigned int h, VkFramebuffer fb)
{
}

void vvgDestroy(const NVGcontext* ctx)
{

}
