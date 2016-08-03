CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)
PROJECT(vvg)

option(BuildExamples "Build the render examples" ON)
option(BuildShaders "Rebuild the shader spirv headers" ON)
option(Shared "Build the library shared" ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

#to disable ninja byproduct warning
cmake_policy(SET CMP0058 NEW)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/external/install/include)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/external/install/lib)

#windows?
if(WIN32 OR MSYS OR MINGW)
	set(VPP_WINDOWS 1)
else()
	set(VPP_WINDOWS 0)
endif()

#vulkan lib
if(${VPP_WINDOWS})
	include_directories("$ENV{VK_SDK_PATH}/Include")
	set(vulkan-lib "$ENV{VK_SDK_PATH}/Source/lib/vulkan-1.dll")
else()
	include_directories("/usr/include/vulkan")
	set(vulkan-lib "vulkan")
endif()

add_subdirectory(src)
add_subdirectory(src/shader)

if(BuildShaders)
	add_subdirectory(external)
endif()

if(BuildExamples)
	add_subdirectory(examples)
endif()
