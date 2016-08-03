# refresh submodules
execute_process(
	COMMAND git submodule update --remote --merge --init
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
	
include(ExternalProject)
set(ExternalProjectCMakeArgs
	-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/install
	-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
	-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
	-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

ExternalProject_Add(vpp_ep
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vpp"
	CMAKE_ARGS ${ExternalProjectCMakeArgs})

ExternalProject_Add_Step(vpp_ep forceconfigure
    COMMAND ${CMAKE_COMMAND} -E echo "Force configure of vpp"
    DEPENDERS build
	DEPENDERS configure
    ALWAYS 1)
