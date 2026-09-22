netlist_inc = include_directories('include')

netlist_library = static_library(
  'ygn-spice-netlist',
  'src/module.cpp',
  include_directories: netlist_inc,
  dependencies: [core_dep, component_library_dep],
)

netlist_dep = declare_dependency(
  include_directories: netlist_inc,
  link_with: netlist_library,
  dependencies: [core_dep, component_library_dep],
)

