project('vvg', ['cpp', 'c'],
	version: '0.1.0',
	meson_version: '>=0.38',
	default_options: ['cpp_std=c++1z'])

add_project_arguments(
	'-Wall',
	'-Wextra',
	'-Wno-unused-parameter',
	'-Wpedantic',
	'-Wno-missing-braces',
	language: 'cpp')

dep_vpp = dependency('vpp', fallback: ['vpp', 'vpp_dep'])

vvg = library('vvg',
  sources: ['src/renderer.cpp', 'src/nanovg.c'],
  dependencies: dep_vpp)

dep_vvg = declare_dependency(
  link_with: vvg,
	include_directories: include_directories('src'),
  dependencies: dep_vpp)

examples = get_option('examples')
if examples
	subdir('examples')
endif
