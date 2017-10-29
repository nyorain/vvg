#include <ny/ny.hpp>
#include <ny/common/gl.hpp>
#include <dlg/dlg.hpp>
#include "nanovg.h"

// GL example for comparison with the vulkan example

#define NANOVG_GL_IMPLEMENTATION 1
#define NANOVG_GL3 1
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include "nanovg-gl.h"

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
		dlg_error("ny backend has no vulkan support!");
		return 0;
	}

	auto ac = backend.createAppContext();

	// ny init
	auto run = true;

	auto listener = MyWindowListener {};
	listener.run = &run;

	auto ws = ny::WindowSettings {};
	ny::GlSurface* surf {};
	ws.surface = ny::SurfaceType::gl;
	ws.listener = &listener;
	ws.size = {width, height};
	ws.gl.storeSurface = &surf;
	auto wc = ac->createWindowContext(ws);

	auto context = ac->glSetup()->createContext();
	context->makeCurrent(*surf);

	// vvg setup
	auto nvgContext = nvgCreateGL3(NVG_ANTIALIAS | NVG_DEBUG);
	auto font = nvgCreateFont(nvgContext, "sans", "Roboto-Regular.ttf");

	using Clock = std::chrono::high_resolution_clock;
	auto lastFrameTimer = Clock::now();
	unsigned int framesCount = 0;
	std::string fpsString = "420 fps";

	// main loop
	while(run) {
		if(!ac->dispatchEvents())
			break;

		glClearColor(0.f, 0.f, 0.f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

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
		nvgPathWinding(nvgContext, NVG_HOLE);
		nvgRect(nvgContext, 700, 400, 300, 300);
		nvgPathWinding(nvgContext, NVG_HOLE);
		// nvgRect(nvgContext, 750, 450, 50, 50);
		// nvgPathWinding(nvgContext, NVG_SOLID);
		// nvgRect(nvgContext, 750, 450, 50, 50);

		// auto paint = nvgRadialGradient(nvgContext, 750, 425,20, 50, nvgRGB(0, 0, 200), nvgRGB(200, 200, 0));
		// auto paint = nvgRadialGradient(nvgContext, 0.0, 0.0, 0.2, 100.0, nvgRGB(0, 0, 200), nvgRGB(200, 200, 0));
		auto paint = nvgLinearGradient(nvgContext, 700, 400, 800, 450, nvgRGB(0, 0, 200), nvgRGB(200, 200, 0));
		nvgFillPaint(nvgContext, paint);
		// nvgFillColor(nvgContext, nvgRGBA(200, 200, 0, 200));
		nvgClosePath(nvgContext);
		nvgFill(nvgContext);

		nvgEndFrame(nvgContext);
		surf->apply();

		// only refresh frame timer every second
		framesCount++;
		if(Clock::now() - lastFrameTimer >= std::chrono::seconds(1)) {
			fpsString = std::to_string(framesCount) + " fps";
			lastFrameTimer = Clock::now();
			framesCount = 0;
		}
	}

	nvgDeleteGL3(nvgContext);
}
