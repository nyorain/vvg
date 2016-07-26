### Vulkan Vector Graphics

vvg is an nanovg backend using the vulkan api licensed under the __MIT License__.
It offers 2 interfaces:

- A high level interface that can be accessed from plain C99 and just provides the nanovg backend.
- A lower level interface written in C++14 that can be used for complex tasks.

The implementation itself is written in C++14 and uses the vpp library.
Currently vvg is in an pre-alpha state, but the first alpha is expected to be released soon.

Any contributions and ideas are appreciated.

### Usage

First choose if you want to use the C or the C++ api.
You can either compile vvg to a static or shared library or compile its single source file
and the nanovg source file with your program/library.
Building can be done using cmake.

Either install or copy the needed header (nanovg_vk.h for the C api or vvg.hpp for the C++ api) as
well as the nanovg header (which is needed to actually draw somthing, vvg is only the backend).
For the C++ api make sure that the vpp headers are in your include path.
For the C api make sure that you include vulkan.h BEFORE including the vvg header.
