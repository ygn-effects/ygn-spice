ygn_spice_app = executable(
  'ygn-spice',
  'main.cpp',
  include_directories: config_inc,
  dependencies: [
    gtkmm_dep,
    cairomm_dep,
    core_dep,
    component_library_dep,
    netlist_dep,
    simulation_dep,
    viewport_dep,
  ],
  install: true,
)

