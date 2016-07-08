function(add_shader name project)
	add_custom_command(
		OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${name}.spv"
		COMMAND glslangValidator
					-V "${CMAKE_CURRENT_SOURCE_DIR}/${name}"
					-o "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${name}.spv"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${name}"
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	COMMENT "Building spirv shader - ${name}")

	add_custom_target(shader_${name} ALL
		DEPENDS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${name}.spv")
	add_dependencies(${project} shader_${name})
endfunction(add_shader)

add_shader("fill.frag" vgk)
add_shader("fill.vert" vgk)

# add_shader("bg.frag" vkg)
# add_shader("bg.vert" vkg)
