project(
  'ygn-spice',
  'cpp',
  default_options: [
    'cpp_std=c++20',
    'warning_level=3',
    'werror=true',
  ],
  meson_version: '>=1.3.0',
)

cpp = meson.get_compiler('cpp')

gtkmm_dep = dependency('gtkmm-4.0', version: '>=4.0')
cairomm_dep = dependency('cairomm-1.16', version: '>=1.16')
catch2_main_dep = dependency('catch2-with-main', version: '>=3.0')
uuid_dep = dependency('uuid', version: '>=2.41.5')

ngspice_option = get_option('ngspice')
ngspice_found = false
ngspice_dep = []

if not ngspice_option.disabled()
  ngspice_candidate = dependency('ngspice', required: false)
  if not ngspice_candidate.found()
    ngspice_candidate = cpp.find_library('ngspice', required: false)
  endif

  ngspice_header_found = cpp.has_header(
    'ngspice/sharedspice.h',
    dependencies: ngspice_candidate,
  )
  ngspice_found = ngspice_candidate.found() and ngspice_header_found

  if ngspice_found
    ngspice_dep = ngspice_candidate
  elif ngspice_option.enabled()
    error('ngspice support was enabled, but libngspice and sharedspice.h were not both found')
  endif
endif

configuration = configuration_data()
configuration.set10('YGN_SPICE_HAVE_NGSPICE', ngspice_found)
configure_file(output: 'config.hpp', configuration: configuration)
config_inc = include_directories('.')

subdir('core')
subdir('library')
subdir('netlist')
subdir('simulation')
subdir('viewport')
subdir('app')
subdir('tests')

summary(
  {
    'C++ standard': 'C++20',
    'gtkmm': gtkmm_dep.version(),
    'cairomm': cairomm_dep.version(),
    'Catch2': catch2_main_dep.version(),
    'uuid': uuid_dep.version(),
    'ngspice backend': ngspice_found,
  },
  section: 'YGN Spice',
)
