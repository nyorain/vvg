dep_ny = dependency('ny', fallback: ['ny', 'ny_dep'])
executable('basic-ny',
  sources: 'basic-ny.cpp',
  dependencies: [dep_ny, dep_vvg])

dep_gl = dependency('gl')
executable('basic-ny-gl',
  sources: ['basic-ny-gl.cpp', '../src/nanovg.c'],
  dependencies: [dep_ny, dep_gl])
