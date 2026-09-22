core_test_sources = files(
  'core/test_module.cpp',
  'core/test_uuid.cpp',
  'core/test_units.cpp',
  'core/test_diagnostic.cpp',
  'core/test_document.cpp',
  'core/test_commands.cpp',
  'core/test_history.cpp',
  'core/test_connectivity.cpp',
)

module_tests = [
  ['core', core_test_sources, core_dep],
  ['library', 'library/test_module.cpp', component_library_dep],
  ['netlist', 'netlist/test_module.cpp', netlist_dep],
  ['simulation', 'simulation/test_module.cpp', simulation_dep],
  ['viewport', 'viewport/test_module.cpp', viewport_dep],
]

foreach module_test : module_tests
  module_name = module_test[0]
  test_executable = executable(
    module_name + '-tests',
    module_test[1],
    dependencies: [catch2_main_dep, module_test[2]],
  )
  test(module_name, test_executable)
endforeach
