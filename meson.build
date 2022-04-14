project(
    'phosphor-inventory-manager', 'cpp',
    version : '1.0.0',
    meson_version: '>=0.58.0',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++20',
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
conf_data.set('CREATE_ASSOCIATIONS', get_option('associations').enabled())
configure_file(output: 'config.h',
    configuration: conf_data
)

sdbusplus_dep = dependency('sdbusplus')
phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')

prog_python = find_program('python3', required: true)

sources = []
deps = []
if get_option('associations').enabled()
    cpp = meson.get_compiler('cpp')
    if cpp.has_header('nlohmann/json.hpp')
        nlohmann_json_dep = declare_dependency()
    else
        subproject('nlohmann', required: false)
        nlohmann_json_dep = declare_dependency(
            include_directories: [
                'subprojects/nlohmann/single_include',
                'subprojects/nlohmann/single_include/nlohmann',
            ]
        )
    endif
    sources += [
        'association_manager.cpp',
    ]
    deps += [
        nlohmann_json_dep,
    ]
endif

generated_cpp = custom_target(
    'generated.cpp',
    command : [
        prog_python,
        meson.project_source_root() + '/pimgen.py',
        '-i', get_option('IFACES_PATH'),
        '-d', get_option('YAML_PATH'),
        '-o', meson.current_build_dir(),
        '-b', '$BUSNAME',
        'generate-cpp'
    ],
    output : 'generated.cpp')

gen_serialization_hpp = custom_target(
    'gen_serialization.hpp',
    command : [
        prog_python,
        meson.project_source_root() + '/pimgen.py',
        '-i', get_option('IFACES_PATH'),
        '-d', get_option('YAML_PATH'),
        '-o', meson.current_build_dir(),
        '-b', '$BUSNAME',
        'generate-serialization'
    ],
    output : 'gen_serialization.hpp')

sources += [
    generated_cpp,
    gen_serialization_hpp,
    'app.cpp',
    'errors.cpp',
    'functor.cpp',
    'manager.cpp',
]

deps += [
    sdbusplus_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
]

executable(
    'phosphor-inventory',
    sources,
    implicit_include_directories: true,
    dependencies: deps,
    install: true,
    install_dir: get_option('bindir'),
)
