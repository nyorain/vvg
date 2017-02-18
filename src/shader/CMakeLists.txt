# bintoheader
ExternalProject_Add(bintoheader_ep
	PREFIX ${CMAKE_BINARY_DIR}/bintoheader
	GIT_REPOSITORY https://github.com/nyorain/bintoheader.git
	GIT_TAG master
	INSTALL_DIR ${CMAKE_BINARY_DIR}/external/install
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/external/install)

ExternalProject_Add_Step(bintoheader_ep
	forceinstall
	DEPENDEES configure
	DEPENDERS install
	ALWAYS 1)

# add_shader
function(add_shader name project)
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
		COMMAND glslangValidator
					-V "${CMAKE_CURRENT_SOURCE_DIR}/${name}"
					-o "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${name}"
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
		OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${name}.spv.h"
		COMMAND "${CMAKE_BINARY_DIR}/external/install/bin/bintoheader"
			-i "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
			-o "${CMAKE_CURRENT_SOURCE_DIR}/${name}.h"
			-n ${name}_data
			-s 32
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		COMMENT "Transforming spirv ${name} into C header")

	add_custom_target(shader_header_${name}
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.spv.h")
	add_dependencies(shader_header_${name} shader_${name})
	add_dependencies(${project} shader_header_${name})

	if(Depend)
		add_dependencies(shader_header_${name} bintoheader_ep)
	endif()
endfunction(add_shader_header)


# shaders
function(add_shader2 name project)
	add_custom_command(
		OUTPUT "${CMAKE_BINARY_DIR}/${name}.spv"
		COMMAND glslangValidator
					-V "${CMAKE_CURRENT_SOURCE_DIR}/${name}"
					-o "${name}.spv"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${name}"
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
		COMMENT "Building spirv shader - ${name}")
	string(REGEX REPLACE \\\. _ array_name ${name})
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${name}.h"
		COMMAND ${CMAKE_BINARY_DIR}/external/install/bin/bintoheader
			-i "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
			-o "${CMAKE_CURRENT_SOURCE_DIR}/${name}.h"
			-s 32
			-n ${array_name}_data
		DEPENDS "${CMAKE_BINARY_DIR}/${name}.spv"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		COMMENT "Generating spirv header - ${name}")
	add_custom_target(shader_${name} ALL
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.h")
	add_dependencies(${project} shader_${name})
endfunction()

# add_shader_header("fill.frag" vvg)
# add_shader_header("fill.vert" vvg)

add_shader2("fill.frag" vvg)
add_shader2("fill.vert" vvg)
