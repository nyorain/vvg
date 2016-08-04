#include <ios>

// This example uses vpp to setup vulkan context and swapChain
#include <vpp/backend/win32.hpp>
#include <vpp/queue.hpp>
#include <vpp/utility/range.hpp>

// This header is always needed to draw something (nanovg context)
#include <nanovg.h>

#include <vvg.hpp> //include this header for the more detailed c++ api
//#include <nanovg_vk.h> //include this header for the high lvl c api

constexpr auto width = 900;
constexpr auto height = 500;

LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
HWND createWindow();

int main()
{
	std::ios_base::sync_with_stdio(false); //especially useful with buggy vulkan layers...
	auto window = createWindow();

	// auto vc = vpp::createContext(window, {width, height}); //default layers and debug flags
	auto vc = vpp::createContext(window, {width, height, {}}); //layers disabled

	// The easy way to create a nanovg context directly from the vpp swapchain
	auto nvgContext = vvg::createContext(vc.swapChain());

	// create a nanovg font for text and fps counter
	auto font = nvgCreateFont(nvgContext, "sans", "Roboto-Regular.ttf");

	//frame timer stuff
	using Clock = std::chrono::high_resolution_clock;
	auto lastFrameTimer = Clock::now();
	unsigned int framesCount = 0;
	std::string fpsString = "420 fps";

	//mainloop
	while(true)
	{
		//dispatch windows messages
	    MSG msg;
	    PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
	    if(msg.message == WM_QUIT)
	    {
	        break;
	    }
	    else
	    {
	        TranslateMessage(&msg);
	        DispatchMessage(&msg);
	    }

		//render
		nvgBeginFrame(nvgContext, width, height, width / (float)height);
		nvgMoveTo(nvgContext, 10, 10);
		nvgLineTo(nvgContext, 10, 400);
		nvgLineTo(nvgContext, 100, 400);
		nvgQuadTo(nvgContext, 100, 50, 400, 120);
		nvgLineTo(nvgContext, 450, 10);
		nvgClosePath(nvgContext);

		nvgFillColor(nvgContext, nvgRGBAf(0.5, 0.8, 0.7, 0.7));
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

		nvgEndFrame(nvgContext);

		//only refresh frame timer every 50ms
		framesCount++;
		if(Clock::now() - lastFrameTimer > std::chrono::milliseconds(50))
		{
			//we multiply it with 20 since its only the fps count of 50ms
			fpsString = std::to_string(framesCount * 20) + " fps";
			lastFrameTimer = Clock::now();
			framesCount = 0;
		}
	}

	//Note that we have to destruct the context even when using the C++ api since
	//the context itself is part of nanovg which is written in C (therefore no RAII).
	vvg::destroyContext(*nvgContext);
}

//The following stuff is not really interesting...
//just windows window creation and mainloop taken from the vpp examples

LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch(message)
    {
		case WM_ERASEBKGND:
		{
			//dont erase it to avoid flickering
			break;
		}

        case WM_CLOSE:
		{
			DestroyWindow(hwnd);
			PostQuitMessage(0);
			break;
		}

        default:
        	return DefWindowProc(hwnd, message, wparam, lparam);
    }

	return 0;
}

HWND createWindow()
{
	std::string name = "Vulkan Vector Graphics";

	WNDCLASSEX wndClass;
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = wndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = GetModuleHandle(nullptr);
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = name.c_str();
	wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	if (!RegisterClassEx(&wndClass))
		throw std::runtime_error("Failed to register window class");

	auto flags = WS_EX_COMPOSITED | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_OVERLAPPEDWINDOW;
	auto window = CreateWindowEx(0, name.c_str(), name.c_str(), flags, CW_USEDEFAULT,
	  CW_USEDEFAULT, width, height, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

	if(!window)
		throw std::runtime_error("Failed to create window");

	COLORREF RRR = RGB(255, 0, 255);
	SetLayeredWindowAttributes(window, RRR, (BYTE)0, LWA_COLORKEY);

	ShowWindow(window, SW_SHOW);
	SetForegroundWindow(window);
	SetFocus(window);

	return window;
}

// The not-so-fancy way to create a nanovg context from plain vulkan handles.
// VVGContextDescription info;
// info.device = (VkDevice)vulkanContext.device().vkDevice();
// info.instance = (VkInstance)vulkanContext.vkInstance();
// info.swapchain = (VkSwapchainKHR)vulkanContext.swapChain().vkHandle();
// info.swapchainSize = vulkanContext.swapChain().size().vkHandle();
// info.swapchainFormat = (VkFormat)vulkanContext.swapChain().format();
// info.phDev = (VkPhysicalDevice)vulkanContext.device().vkPhysicalDevice();
// info.queue = (VkQueue)vulkanContext.device().queues()[0]->vkHandle();
// info.queueFamily = vulkanContext.device().queues()[0]->family();
// auto nvgContext = vvgCreate(&info);
