#vkg
add_library(vgk renderer.cpp ../external/nanovg/src/nanovg.c)
target_link_libraries(vgk vpp ${vulkan-lib})

add_dependencies(vgk nanovg_ep vpp_ep)

#copy runtime files
add_custom_command(
	TARGET vgk POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_BINARY_DIR}/external/install/bin/."
		"${CMAKE_BINARY_DIR}/bin"
	COMMENT "copying bin files")
