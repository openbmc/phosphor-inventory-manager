project(
    'phosphor-inventory-manager', 'cpp',
    version : '1.0.0',
    meson_version: '>=1.1.1',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++23',
        'buildtype=debugoptimized',
    ]
)

conf_data = configuration_data()
conf_data.set_quoted('BUSNAME', 'xyz.openbmc_project.Inventory.Manager')
conf_data.set_quoted('INVENTORY_ROOT', '/xyz/openbmc_project/inventory')
conf_data.set_quoted('IFACE', 'xyz.openbmc_project.Inventory.Manager')
conf_data.set_quoted('PIM_PERSIST_PATH', '/var/lib/phosphor-inventory-manager')
conf_data.set_quoted('ASSOCIATIONS_FILE_PATH', '/usr/share/phosphor-inventory-manager/associations.json')
conf_data.set('CLASS_VERSION', 2)
conf_data.set('CREATE_ASSOCIATIONS', get_option('associations').allowed())
configure_file(output: 'config.h',
    configuration: conf_data
)

cpp = meson.get_compiler('cpp')
# Get Cereal dependency.
cereal_dep = dependency('cereal', required: false)
has_cereal = cpp.has_header_symbol(
    'cereal/cereal.hpp',
    'cereal::specialize',
    dependencies: cereal_dep,
    required: false)
if not has_cereal
    cereal_opts = import('cmake').subproject_options()
    cereal_opts.add_cmake_defines({'BUILD_TESTS': 'OFF'})
    cereal_proj = import('cmake').subproject(
        'cereal',
        options: cereal_opts,
        required: false)
    assert(cereal_proj.found(), 'cereal is required')
    cereal_dep = cereal_proj.dependency('cereal')
endif

sdbusplus_dep = dependency('sdbusplus')
phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')

prog_python = find_program('python3', required: true)

sources = []
deps = []
if get_option('associations').allowed()
    sources += [
        'association_manager.cpp',
    ]
    deps += [
        dependency('nlohmann_json', include_type: 'system'),
    ]
endif

ifacesdir = get_option('IFACES_PATH')
if ifacesdir == ''
    ifacesdir = phosphor_dbus_interfaces_dep.get_variable('yamldir')
endif

generated_cpp = custom_target(
    'generated.cpp',
    command : [
        prog_python,
        meson.project_source_root() + '/pimgen.py',
        '-i', ifacesdir,
        '-d', get_option('YAML_PATH'),
        '-o', meson.current_build_dir(),
        '-b', conf_data.get_unquoted('BUSNAME'),
        'generate-cpp'
    ],
    output : 'generated.cpp')

gen_serialization_hpp = custom_target(
    'gen_serialization.hpp',
    command : [
        prog_python,
        meson.project_source_root() + '/pimgen.py',
        '-i', ifacesdir,
        '-d', get_option('YAML_PATH'),
        '-o', meson.current_build_dir(),
        '-b', conf_data.get_unquoted('BUSNAME'),
        'generate-serialization'
    ],
    output : 'gen_serialization.hpp')

sources += [
    generated_cpp,
    gen_serialization_hpp,
    'app.cpp',
    'errors.cpp',
    'remove_association.cpp',
    'functor.cpp',
    'manager.cpp',
]

deps += [
    cereal_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdbusplus_dep,
]

executable(
    'phosphor-inventory',
    sources,
    implicit_include_directories: true,
    dependencies: deps,
    install: true,
    install_dir: get_option('bindir'),
)

build_tests = get_option('tests')
if not build_tests.disabled()
    subdir('test')
endif
