#include <ny/ny.hpp>
#include <vpp/swapchain.hpp>
#include <vpp/debug.hpp>
#include <vpp/device.hpp>
#include <vpp/instance.hpp>
#include <vvg.hpp>
#include <nanovg.h>

#define WithLayers 1

class MyWindowListener : public ny::WindowListener {
public:
	bool* run;

	void close(const ny::CloseEvent& ev) override {
		*run = false;
	}
	void key(const ny::KeyEvent& ev) override {
		if(ev.pressed && ev.keycode == ny::Keycode::escape)
			*run = false;
	}
};

int main()
{
	constexpr auto width = 1200u;
	constexpr auto height = 800u;

	// init ny app
	auto& backend = ny::Backend::choose();
	if(!backend.vulkan()) {
		ny::error("ny backend has no vulkan support!");
		return 0;
	}

	auto ac = backend.createAppContext();

	// basic vpp init
	auto iniExtensions = ac->vulkanExtensions();
	iniExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	vk::ApplicationInfo appInfo ("vpp-intro", 1, "vpp", 1, VK_API_VERSION_1_0);
	vk::InstanceCreateInfo instanceInfo;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = iniExtensions.size();
	instanceInfo.ppEnabledExtensionNames = iniExtensions.data();

#ifdef WithLayers
	constexpr auto layer = "VK_LAYER_LUNARG_standard_validation";
	instanceInfo.enabledLayerCount = 1;
	instanceInfo.ppEnabledLayerNames = &layer;
#endif

	vpp::Instance instance(instanceInfo);

#ifdef WithLayers
	vpp::DebugCallback debugCallback(instance);
#endif

	// ny init
	auto run = true;

	auto listener = MyWindowListener {};
	listener.run = &run;

	auto vkSurface = vk::SurfaceKHR {};
	auto ws = ny::WindowSettings {};
	ws.surface = ny::SurfaceType::vulkan;
	ws.listener = &listener;
	ws.size = {width, height};
	ws.vulkan.instance = (VkInstance) instance.vkHandle();
	ws.vulkan.storeSurface = &(std::uintptr_t&) (vkSurface);
	auto wc = ac->createWindowContext(ws);

	// further vpp init
	const vpp::Queue* presentQueue;
	vpp::Device device(instance, vkSurface, presentQueue);
	vpp::Swapchain swapchain(device, vkSurface, {width, height}, {});

	// vvg setup
	auto nvgContext = vvg::createContext(swapchain);
	auto font = nvgCreateFont(nvgContext, "sans", "Roboto-Regular.ttf");

	using Clock = std::chrono::high_resolution_clock;
	auto lastFrameTimer = Clock::now();
	unsigned int framesCount = 0;
	std::string fpsString = "420 fps";

	// main loop
	while(run) {
		if(!ac->dispatchEvents())
			break;

		nvgBeginFrame(nvgContext, width, height, width / (float) height);

		nvgBeginPath(nvgContext);
		nvgMoveTo(nvgContext, 10, 10);
		nvgLineTo(nvgContext, 10, 400);
		nvgLineTo(nvgContext, 100, 400);
		nvgQuadTo(nvgContext, 100, 50, 400, 120);
		nvgLineTo(nvgContext, 450, 10);
		nvgClosePath(nvgContext);

		nvgFillColor(nvgContext, nvgRGBAf(0.5, 0.8, 0.7, 1.0));
		nvgFill(nvgContext);

		nvgBeginPath(nvgContext);
		nvgFontFaceId(nvgContext, font);
		nvgFontSize(nvgContext, 100.f);
		nvgFontBlur(nvgContext, .8f);
		nvgFillColor(nvgContext, nvgRGBAf(1.0, 1.0, 1.0, 1.0));
		nvgTextBox(nvgContext, 200, 200, width - 200, "Hello Vulkan Vector Graphics World", nullptr);

		nvgFontSize(nvgContext, 30.f);
		nvgFontBlur(nvgContext, .2f);
		nvgText(nvgContext, 10, height - 20, fpsString.c_str(), nullptr);

		nvgBeginPath(nvgContext);
		nvgRect(nvgContext, 700, 400, 100, 50);
		// auto paint = nvgRadialGradient(nvgContext, 750, 425,20, 50, nvgRGB(0, 0, 200), nvgRGB(200, 200, 0));
		// auto paint = nvgRadialGradient(nvgContext, 0.0, 0.0, 0.2, 100.0, nvgRGB(0, 0, 200), nvgRGB(200, 200, 0));
		auto paint = nvgLinearGradient(nvgContext, 700, 400, 800, 450, nvgRGB(0, 0, 200), nvgRGB(200, 200, 0));
		nvgFillPaint(nvgContext, paint);
		// nvgFillColor(nvgContext, nvgRGBA(200, 200, 0, 200));
		nvgClosePath(nvgContext);
		nvgFill(nvgContext);

		nvgEndFrame(nvgContext);

		// only refresh frame timer every second
		framesCount++;
		if(Clock::now() - lastFrameTimer >= std::chrono::seconds(1)) {
			fpsString = std::to_string(framesCount) + " fps";
			lastFrameTimer = Clock::now();
			framesCount = 0;
		}
	}

	vvg::destroyContext(*nvgContext);
}
