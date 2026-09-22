viewport_inc = include_directories('include')

viewport_library = static_library(
  'ygn-spice-viewport',
  'src/module.cpp',
  include_directories: viewport_inc,
  dependencies: core_dep,
)

viewport_dep = declare_dependency(
  include_directories: viewport_inc,
  link_with: viewport_library,
  dependencies: core_dep,
)

