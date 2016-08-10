### Vulkan Vector Graphics

vvg is a __[nanovg]__ backend using the [vulkan] api licensed under the __MIT License__.
It offers 2 interfaces:

- A [high level interface] that can be accessed from plain C99 and just provides the nanovg backend.
- A [lower level interface] written in C++14 that can be used for complex tasks and is perfectly integrated with vpp.

The implementation itself is written in C++14 and uses the [vpp] library. It also makes use of the [bintoheader] tool
to include the compiles spirv binaries directly into the source code. See for example [src/shader/headers/fill.frag.h].

Currently vvg is in an pre-alpha state, but the first alpha is expected to be released soon. Note that it is not well tested yet, so there will be probably bugs and issues on different drivers. Was only tested/developed using MinGW64 (gcc 5/6) with an AMD R9 270x.
Any bug reports, contributions and ideas are highly appreciated.

If you make a pull request, do not forget to append yourself to the contributors section, no matter how small the actual contribution is.

### Usage

You have to choose if you want to use the C or the C++ api.
You can either compile vvg to a static or shared library or compile its single source file
and the [nanovg] source file with your program/library. Note that the implementation needs to be linked to the latest
[vpp] if you choose to just built the implementation source togehter with you project.

Building can be done using cmake. vvg has __no external dependencies except vulkan__ and will pull everyhting it needs itself.

Either install or copy the needed header ([nanovg_vk.h] for the C api or [vvg.hpp] for the C++ api) as
well as the [nanovg] header which can also be found in the [src/] directory (nanovg.h, needed to actually draw something,
vvg is only the backend).
For the C++ api make sure that the [vpp] headers are in your include path.
For the C api make sure that you include vulkan.h BEFORE including the vvg header. You still have to link with [vpp].

For an example how to use it see [examples/example.cpp]. 
It has only support for Windows at the moment (since it creates a window for rendering).

## Contributors

*nyorain*

[examples/example.cpp]: examples/example.cpp
[vulkan]: https://www.khronos.org/vulkan/
[high level interface]: src/nanovg_vk.h
[nanovg_vk.h]: src/nanovg_vk.h
[lower level interface]: src/vvg.hpp
[vvg.hpp]: src/vvg.hpp
[src/]: src/
[bintoheader]: https://github.com/nyorain/bintoheader
[vpp]: https://github.com/nyorain/vpp
[nanovg]: https://github.com/memononen/nanovg
[src/shader/headers/fill.frag.h]: src/shader/headers/fill.frag.h
