# add_shader
function(add_shader name project)
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
		COMMAND glslangValidator
					-V "${CMAKE_CURRENT_SOURCE_DIR}/src/${name}"
					-o "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/${name}"
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
		COMMENT "Building spirv shader - ${name}")

	add_custom_target(shader_${name}
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv")
	add_dependencies(${project} shader_${name})
endfunction(add_shader)

# add_shader_header
function(add_shader_header name project)
	add_shader(${name} ${project})
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/headers/${name}.spv.h"
		COMMAND "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bintoheader"
			-i "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
			-o "${CMAKE_CURRENT_SOURCE_DIR}/headers/${name}.h"
			-n ${name}_data
			-s 32
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/headers"
		COMMENT "Transforming spirv ${name} into C header")

	add_custom_target(shader_header_${name} 
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/headers/${name}.spv.h")
	add_dependencies(shader_header_${name} bintoheader)
	add_dependencies(shader_header_${name} shader_${name})
	add_dependencies(${project} shader_header_${name})
endfunction(add_shader_header)

add_executable(bintoheader bintoheader.cpp)

add_shader_header("fill.frag" vvg)
add_shader_header("fill.vert" vvg)
