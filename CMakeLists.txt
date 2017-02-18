CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)
PROJECT(vvg)

option(Debug "Build in debug mode" on)
option(Examples "Build the render examples (windows-only)" on)
option(Shared "Build the library shared" on)

option(Depend "Pull in dependencies every built. Development opetion" on)
option(Shaders "Rebuild the shader spirv headers. Development option" on)

set(CMAKE_BUILD_TYPE Debug)

# set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")

# to disable ninja byproduct warning
cmake_policy(SET CMP0058 NEW)

# enable warnings
set(is_clang CMAKE_CXX_COMPILER_ID MATCHES "Clang")
if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR is_clang)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -Wextra -Wpedantic")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/external/install/include)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/external/install/lib)

# external projects
include(ExternalProject)
set(ExternalProjectCMakeArgs
	-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/external/install
	-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
	-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DDebug=${Debug})

# vpp (using vulkan 1.0.20 api)
ExternalProject_Add(vpp_ep
	PREFIX ${CMAKE_INARY_DIR}/vpp
	GIT_REPOSITORY https://github.com/nyorain/vpp.git
	GIT_TAG master
	INSTALL_DIR ${CMAKE_BINARY_DIR}/external/install
	CMAKE_ARGS ${ExternalProjectCMakeArgs}
		-DApiVersion=1-0-20)
ExternalProject_Add_Step(vpp_ep
	forceinstall
	DEPENDEES configure
	DEPENDERS install
	ALWAYS 1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(Vulkan REQUIRED)

add_subdirectory(src)

if(Shaders)
	add_subdirectory(src/shader)
endif()

if(Examples)
	if(Win32 OR MSYS OR MINGW)
		add_subdirectory(examples)
	else()
		message(WARNING "The examples are windows-only at the moment.")
	endif()
endif()
