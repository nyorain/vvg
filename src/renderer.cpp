#include <nanovg_vk.h>
#include <uniformData.hpp>

#include <nanovg/nanovg.h>

#include <vpp/device.hpp>
#include <vpp/swapChain.hpp>
#include <vpp/graphicsPipeline.hpp>
#include <vpp/buffer.hpp>
#include <vpp/provider.hpp>
#include <vpp/image.hpp>
#include <vpp/descriptor.hpp>
#include <vpp/renderer.hpp>
#include <vpp/renderPass.hpp>
#include <vpp/bufferOps.hpp>
#include <vpp/vk.hpp>
#include <vpp/utility/range.hpp>

#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <iterator>

//implementation TODO:
//-try to use push constants for uniform (check maxPushConstantsSize)

namespace vgk
{

class Texture;
class Renderer
{
public:
	Renderer(const vpp::SwapChain& swapChain, bool antiAlias);
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
	void initRenderPass(const vpp::SwapChain& swapChain);
	void record(vk::CommandBuffer buffer);

	DrawData& parsePaint(const NVGpaint& paint, const NVGscissor& scissor, float fringe,
		float strokeWidth);

	const vpp::CommandBuffer& commandBuffer() const;
	const vpp::Device& device() const { return swapChain_->device(); }

protected:
	unsigned int texID_ = 0;
	std::vector<Texture> textures_;

	unsigned int width_;
	unsigned int height_;

	vpp::Buffer uniformBuffer_;
	vpp::Buffer vertexBuffer_;

	std::vector<DrawData> drawDatas_;
	std::vector<NVGvertex> vertices_;

	vpp::DescriptorPool descriptorPool_ {};
	vpp::DescriptorSetLayout descriptorLayout_;
	unsigned int descriptorPoolSize_ {};

	//vpp::PipelineLayout layout_;
	vpp::PipelineLayout pipelineLayout_;
	vpp::Pipeline fanPipeline_;
	vpp::Pipeline stripPipeline_;
	vpp::Pipeline listPipeline_;
	int bound_ = 0; //1: fan, 2: strip, 3: triangles

	vk::Sampler sampler_;

	const vpp::SwapChain* swapChain_ {};
	vpp::SwapChainRenderer renderer_;
	vpp::RenderPass renderPass_;

	bool edgeAA_ = true;
};

class Texture
{
public:
	Texture(Renderer& renderer, unsigned int xid, unsigned int format, unsigned int w,
		unsigned int h, int flags, const std::uint8_t& data);
	~Texture() = default;

	Texture(Texture&& other) noexcept = default;
	Texture& operator=(Texture&& other) noexcept = default;

	unsigned int id() const { return id_; }
	unsigned int width() const { return width_; }
	unsigned int height() const { return height_; }
	const vpp::ViewableImage& image() const { return image_; }

	void update(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
		const std::uint8_t& data);
	unsigned int texType() const { return components_ == 1 ? 2 : 1; }

protected:
	unsigned int id_;
	unsigned int width_;
	unsigned int height_;

	vpp::ViewableImage image_;
	unsigned int components_;
};

extern const NVGparams nvgContextImpl;

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

} //end anonymous namespace

//create function
NVGcontext* create(const vpp::SwapChain& swapChain, bool antiAlias)
{
	auto renderer = new Renderer(swapChain, antiAlias);

	auto params = nvgContextImpl;
	params.userPtr = renderer;
	params.edgeAntiAlias = antiAlias;

	auto ret = nvgCreateInternal(&params);

	if(!ret) delete renderer;
	return ret;
}

void destroy(NVGcontext& context)
{
	nvgDeleteInternal(&context);
}

//Renderer
Renderer::Renderer(const vpp::SwapChain& swapChain, bool antiAlias)
	: swapChain_(&swapChain), edgeAA_(antiAlias)
{
	constexpr auto uniformSize = 8 + 4 * 2 + 16 * 2 + 64 * 2; //see fill.frag
	constexpr auto vertexSize = 5 * 1024 * 1024; //5MB, just a guess
	auto& dev = swapChain.device();

	initRenderPass(swapChain);

	//sampler
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::linear;
	samplerInfo.minFilter = vk::Filter::linear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::linear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::repeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::repeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::repeat;
	samplerInfo.mipLodBias = 0;
	samplerInfo.anisotropyEnable = true;
	samplerInfo.maxAnisotropy = 8;
	samplerInfo.compareEnable = false;
	samplerInfo.compareOp = {};
	samplerInfo.minLod = 0;
	samplerInfo.maxLod = 0;
	samplerInfo.borderColor = vk::BorderColor::floatTransparentBlack;
	samplerInfo.unnormalizedCoordinates = false;
	sampler_ = vk::createSampler(device(), samplerInfo);

	//descLayout
	auto descriptorBindings  =
	{
		vpp::descriptorBinding(vk::DescriptorType::uniformBuffer,
			vk::ShaderStageBits::vertex | vk::ShaderStageBits::fragment),
		vpp::descriptorBinding(vk::DescriptorType::combinedImageSampler,
			vk::ShaderStageBits::fragment, -1, 1, &sampler_)
	};

	descriptorLayout_ = {dev, descriptorBindings};
	pipelineLayout_ = {dev, {descriptorLayout_}};

	//create the graphics pipeline
	vpp::GraphicsPipelineBuilder builder(dev, renderPass_);
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

	//layouts
	//vertex
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

	builder.shader.stage("bin/shaders/fill.vert.spv", {vk::ShaderStageBits::vertex});
	builder.shader.stage("bin/shaders/fill.frag.spv", {vk::ShaderStageBits::fragment, &specInfo});

	fanPipeline_ = builder.build();

	builder.states.inputAssembly.topology = vk::PrimitiveTopology::triangleStrip;
	stripPipeline_ = builder.build();

	builder.states.inputAssembly.topology = vk::PrimitiveTopology::triangleList;
	listPipeline_ = builder.build();

	//create the swap chain renderer
	auto impl = std::make_unique<RenderImpl>();
	impl->renderer = this;
	impl->swapChainRenderer = &renderer_;

	//stencil attachment
	auto attachmentInfo = vpp::ViewableImage::defaultDepth2D();
	attachmentInfo.imgInfo.format = vk::Format::s8Uint;
	attachmentInfo.viewInfo.format = vk::Format::s8Uint;
	attachmentInfo.viewInfo.subresourceRange.aspectMask = vk::ImageAspectBits::stencil;

	vpp::SwapChainRenderer::CreateInfo info {renderPass_, 0, {{attachmentInfo}}};
	renderer_ = vpp::SwapChainRenderer(swapChain, info, std::move(impl));
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

	return 1;
}

void Renderer::viewport(int width, int height)
{
	//store (and set) viewport in some way
	width_ = width;
	height_ = height;
}

void Renderer::cancel()
{
	vertices_.clear();
	drawDatas_.clear();
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
	else
	{
		vk::resetDescriptorPool(device(), descriptorPool_, {});
	}

	//update
	vpp::BufferUpdate update(uniformBuffer_, vpp::BufferLayout::std140);
	for(auto& data : drawDatas_)
	{
		auto offset = update.offset();
		update.add(vpp::raw(data.uniformData));

		data.descriptorSet = vpp::DescriptorSet(descriptorLayout_, descriptorPool_);

		vpp::DescriptorSetUpdate descUpdate(data.descriptorSet);
		descUpdate.uniform({{uniformBuffer_, offset, sizeof(UniformData)}});

		if(data.texture != 0)
		{
			auto* tex = texture(data.texture);
			descUpdate.imageSampler({{{}, tex->image().vkImageView(), vk::ImageLayout::general}});
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

		data.uniformData.type = typeTexture;
		if(tex) data.uniformData.texType = tex->texType();

		data.texture = paint.image;
	}
	else if(paint.innerColor == paint.outerColor)
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

	//stencil attachment
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

	renderPass_ = vpp::RenderPass(dev, renderPassInfo);
}

namespace
{

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
	//or: return 0;

	tex->update(x, y, w, h, *data);
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




//texture
Texture::Texture(Renderer& renderer, unsigned int xid, unsigned int format, unsigned int w,
	unsigned int h, int flags, const std::uint8_t& data) : id_(xid), width_(w), height_(h)
{
	std::cout << "image: " << w << " " << h << "\n";
	vk::Format vkformat = vk::Format::r8g8b8a8Unorm;
	components_ = 4;

	if(format == NVG_TEXTURE_ALPHA)
	{
		// std::cout << "alphaTexture\n";
		vkformat = vk::Format::r8Unorm;
		components_ = 1;
	}

	vk::Extent3D extent {w, h, 1};

	auto info = vpp::ViewableImage::defaultColor2D();
	info.imgInfo.extent = extent;
	info.imgInfo.initialLayout = vk::ImageLayout::preinitialized;
	info.imgInfo.tiling = vk::ImageTiling::linear;
	info.imgInfo.format = vkformat;
	info.viewInfo.format = vkformat;
	info.imgInfo.usage = vk::ImageUsageBits::transferDst | vk::ImageUsageBits::sampled;
	// info.memoryFlags = vk::MemoryPropertyBits::hostVisible;
	image_ = vpp::ViewableImage(renderer.device(), info);

	//XXX TODO: make param pointer since it might be nullptr
	if(&data)
	{
		vpp::fill(image_.image(), data, vkformat, vk::ImageLayout::preinitialized, extent,
			{vk::ImageAspectBits::color, 0, 0});
	}
}

void Texture::update(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
	const std::uint8_t& data)
{
	w = 512;
	h = 512;

	std::cout << "update: " << x << " " << y << " " << w << " " << h << "\n";
	vk::Extent3D extent {w, h, 1};
	vk::Offset3D offset {int(x), int(y), 0};


	std::cout << "siiize: " << w * h * components_ << "\n";
	std::cout << image_.image().size() << "\n";

	auto format = vk::Format::r8g8b8a8Unorm;
	if(components_ == 1) format = vk::Format::r8Unorm;

	std::vector<std::uint8_t> data1(w * h * components_, 200);

	///XXX: offset!
	vpp::fill(image_.image(), data, format, vk::ImageLayout::preinitialized, extent,
		{vk::ImageAspectBits::color, 0, 0}, offset)->finish();
}

}
