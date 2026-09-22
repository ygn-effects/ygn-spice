component_library_inc = include_directories('include')

component_library = static_library(
  'ygn-spice-library',
  'src/module.cpp',
  include_directories: component_library_inc,
  dependencies: core_dep,
)

component_library_dep = declare_dependency(
  include_directories: component_library_inc,
  link_with: component_library,
  dependencies: core_dep,
)

