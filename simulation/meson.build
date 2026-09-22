simulation_inc = include_directories('include')

simulation_dependencies = [core_dep, netlist_dep]
if ngspice_found
  simulation_dependencies += [ngspice_dep]
endif

simulation_library = static_library(
  'ygn-spice-simulation',
  'src/module.cpp',
  include_directories: simulation_inc,
  dependencies: simulation_dependencies,
)

simulation_dep = declare_dependency(
  include_directories: simulation_inc,
  link_with: simulation_library,
  dependencies: simulation_dependencies,
)
