file(COPY "Roboto-Regular.ttf" DESTINATION ${CMAKE_BINARY_DIR})
file(COPY "Roboto-Regular.ttf" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# add_executable(basic basic-windows.cpp)
# target_link_libraries(basic vpp vvg)

add_executable(offscreen offscreen.cpp)
target_link_libraries(offscreen vpp vvg)


# example using ny
ExternalProject_Add(ny_ep
	PREFIX ${CMAKE_BINARY_DIR}/ny
	GIT_REPOSITORY https://github.com/nyorain/ny.git
	GIT_TAG master
	INSTALL_DIR ${CMAKE_BINARY_DIR}/external/install
	CMAKE_ARGS ${ExternalProjectCMakeArgs})

ExternalProject_Add_Step(ny_ep
	forceinstall
	DEPENDEES configure
	DEPENDERS install
	ALWAYS 1)

add_executable(basic-ny basic-ny.cpp)
target_link_libraries(basic-ny vpp vvg ny)

if(Depend)
	add_dependency(basic-ny ny_ep)
endif()

add_custom_command(
	TARGET basic-ny POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_BINARY_DIR}/external/install/bin/."
		"${CMAKE_BINARY_DIR}/bin"
	COMMENT "Copying external binary files")
