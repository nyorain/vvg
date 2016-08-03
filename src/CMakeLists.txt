#vkg
if(Shared)
	add_library(vvg SHARED renderer.cpp nanovg.c)
else()
	add_library(vvg renderer.cpp nanovg.c)
endif()

target_link_libraries(vvg vpp ${vulkan-lib})

add_dependencies(vvg vpp_ep)

#copy runtime files
add_custom_command(
	TARGET vvg PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_BINARY_DIR}/external/install/bin/."
		"${CMAKE_BINARY_DIR}/bin"
		COMMENT "Copying vpp bin files")

#install
install(TARGETS vvg
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)
