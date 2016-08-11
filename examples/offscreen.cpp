#include <vvg.hpp>

#include <vpp/framebuffer.hpp>
#include <vpp/image.hpp>
#include <vpp/device.hpp>
#include <vpp/instance.hpp>
#include <vpp/debug.hpp>
#include <vpp/vk.hpp>

#include <nanovg.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

vpp::RenderPass initRenderPass(const vpp::Device& dev, vk::Format format);

int main()
{
	//init vulkan stuff
	//dont use a context here since we dont need surface/swapChain
	//most of this is taken somehow from vpp/src/context.cpp

	//instance
	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "vvg-offscreen";
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = "vvg";
	appInfo.engineVersion = 1;
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 21);

	vk::InstanceCreateInfo iniinfo;
	iniinfo.enabledLayerCount = vpp::validationLayerNames.size();
	iniinfo.ppEnabledLayerNames = vpp::validationLayerNames.data();
	iniinfo.enabledExtensionCount = 0;
	iniinfo.ppEnabledExtensionNames = nullptr;
	iniinfo.pApplicationInfo = &appInfo;

	vpp::Instance instance(iniinfo);

	//device
	auto phdevs = vk::enumeratePhysicalDevices(instance);
	auto queueProps = vk::getPhysicalDeviceQueueFamilyProperties(phdevs[0]);

	const float prio = 0.0;
	vk::DeviceQueueCreateInfo queueInfo;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &prio;
	for(auto i = 0u; i < queueProps.size(); ++i)
	{
		const auto& qProp = queueProps[i];
		queueInfo.queueFamilyIndex = i;
		if(qProp.queueFlags & vk::QueueBits::graphics) break;
	}

	vk::DeviceCreateInfo devinfo;
	devinfo.queueCreateInfoCount = 1;
	devinfo.pQueueCreateInfos = &queueInfo;
	devinfo.enabledLayerCount = vpp::validationLayerNames.size();
	devinfo.ppEnabledLayerNames = vpp::validationLayerNames.data();
	devinfo.enabledExtensionCount = 0;
	devinfo.ppEnabledExtensionNames = nullptr;

	vpp::Device dev(instance, phdevs[0], devinfo);

	//renderPass
	auto renderPass = initRenderPass(dev, vk::Format::r8g8b8a8Unorm);

	//framebuffer
	auto width = 1024;
	auto height = 1024;

	auto colorAttachment = vpp::ViewableImage::defaultColor2D();
	// colorAttachment.imgInfo.extent = {width, height, 1};
	colorAttachment.imgInfo.usage |= vk::ImageUsageBits::colorAttachment | vk::ImageUsageBits::transferSrc;
	colorAttachment.imgInfo.format = vk::Format::r8g8b8a8Unorm;
	colorAttachment.imgInfo.tiling = vk::ImageTiling::linear;
	colorAttachment.viewInfo.format = vk::Format::r8g8b8a8Unorm;
	colorAttachment.memoryFlags = vk::MemoryPropertyBits::hostVisible;

	auto depthAttachment = vpp::ViewableImage::defaultDepth2D();

	vpp::Framebuffer framebuffer(dev, renderPass, {width, height},
		{colorAttachment, depthAttachment});

	//create the nanovg context
	auto nvgContext = vvg::createContext(framebuffer, renderPass);
	nvgBeginFrame(nvgContext, width, height, width / (float)height);

	nvgBeginPath(nvgContext);
	nvgMoveTo(nvgContext, 10, 10);
	nvgLineTo(nvgContext, 10, 400);
	nvgLineTo(nvgContext, 100, 400);
	nvgQuadTo(nvgContext, 100, 50, 400, 120);
	nvgLineTo(nvgContext, 450, 10);
	nvgClosePath(nvgContext);
	nvgFillColor(nvgContext, nvgRGBAf(0.5, 0.8, 0.7, 0.7));
	nvgFill(nvgContext);

	nvgEndFrame(nvgContext);

	vvg::destroyContext(*nvgContext);

	//write the image to a file
	auto ptr = vpp::retrieve(framebuffer.attachments()[0].image(), vk::ImageLayout::general,
		vk::Format::r8g8b8a8Unorm, {width, height, 1}, {vk::ImageAspectBits::color, 0, 0});
	auto& data = ptr->data();
	return stbi_write_png("test.png", width, height, 4, &data, width * 4);
}

vpp::RenderPass initRenderPass(const vpp::Device& dev, vk::Format format)
{
	vk::AttachmentDescription attachments[2] {};

	//color from swapchain
	attachments[0].format = format;
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

	return {dev, renderPassInfo};
}
