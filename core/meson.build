core_inc = include_directories('include')

core_library = static_library(
  'ygn-spice-core',
  'src/module.cpp',
  'src/uuid.cpp',
  'src/units.cpp',
  'src/document.cpp',
  'src/connectivity.cpp',
  'src/commands.cpp',
  'src/history.cpp',
  include_directories: core_inc,
  dependencies: uuid_dep,
)

core_dep = declare_dependency(
  include_directories: core_inc,
  link_with: core_library,
  dependencies: uuid_dep,
)
