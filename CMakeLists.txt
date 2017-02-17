CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)
PROJECT(vvg)

option(BuildExamples "Build the render examples" on)
option(BuildShaders "Rebuild the shader spirv headers" off)
option(Shared "Build the library shared" on)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

#to disable ninja byproduct warning
cmake_policy(SET CMP0058 NEW)

# enable warnings
# for msvc we first check if some level is set by default and replace it
if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
	  list(APPEND CMAKE_CXX_FLAGS "/W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	list(APPEND CMAKE_CXX_FLAGS "-Wall -Wno-long-long")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/external/install/include)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/external/install/lib)

add_subdirectory(external)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/vpp/src/cmake")
find_package(Vulkan REQUIRED)

add_subdirectory(src)

if(BuildShaders)
	add_subdirectory(src/shader)
endif()

if(BuildExamples)
	if(Win32 OR MSYS OR MINGW)
		add_subdirectory(examples)
	else()
		message("The examples are windows-only at the moment. Pull requests appreciated.")
	endif()
endif()
