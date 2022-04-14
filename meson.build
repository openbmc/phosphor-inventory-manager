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
sdbusplusplus_prog = find_program('sdbus++', required : true)
sdbuspp_gen_meson_prog = find_program('sdbus++-gen-meson', required : true)
realpath_prog = find_program('realpath')

selected_subdirs = []
dir = get_option('YAML_PATH') + '/extra_interfaces.d/'
selected_subdirs += dir + 'xyz/openbmc_project'

generated_root = meson.current_build_dir() / 'gen'
generated_others = []
generated_sources = []

# Source the generated meson files.
subdir('gen')
foreach d : selected_subdirs
  subdir('gen' / d.replace(dir, ''))
endforeach

libmanagercommon_source = []
libmanagercommon_dep = []
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
    libmanagercommon_source = [
        'association_manager.cpp',
    ]
    libmanagercommon_dep = [
        nlohmann_json_dep,
    ]
endif

# Parse through the list from sdbus++-gendir and put into sets.
generated_headers = []
generated_cpp = []
generated_others_files = []

foreach g : generated_sources generated_others
    foreach f : g.to_list()
        rel_path = run_command(
            realpath_prog,
            '--relative-to', generated_root,
            f.full_path(),
        ).stdout().strip().split('\n')[-1]

        if rel_path.endswith('.hpp')
            generated_headers += rel_path
        elif rel_path.endswith('.cpp')
            generated_cpp += rel_path
        else
            generated_others_files += rel_path
        endif
    endforeach
endforeach

generated_cpp = custom_target(
    'generated.cpp',
    command : [
        prog_python,
        meson.project_source_root() + '/pimgen.py',
        '-i', meson.project_source_root() + '/' + get_option('IFACES_PATH'),
        '-d', meson.project_source_root() + '/' + get_option('YAML_PATH'),
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
        '-d', meson.project_source_root() + '/' + get_option('YAML_PATH'),
        '-o', meson.current_build_dir(),
        '-b', '$BUSNAME',
        'generate-serialization'
    ],
    output : 'gen_serialization.hpp')

sources = [
    generated_cpp,
    gen_serialization_hpp,
    'app.cpp',
]

deps = [
    sdbusplus_dep,
]

libmanagercommon_source += [
    'errors.cpp',
    'functor.cpp',
    'manager.cpp',
]

libmanagercommon_dep += [
    sdbusplus_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
]

libmanagercommon = static_library(
    'managercommon',
    libmanagercommon_source,
    dependencies: libmanagercommon_dep,
    implicit_include_directories: false,
    include_directories: ['./', '../', './gen'],
)

libmanager_source = [
    generated_cpp,
    generated_sources,
]

libmanager = static_library(
    'manager',
    libmanager_source,
    dependencies: [sdbusplus_dep],
    link_with: [ libmanagercommon ],
    implicit_include_directories: false,
    include_directories: ['./', '../', './gen'],
)

executable(
    'phosphor-inventory',
    sources,
    generated_sources,
    implicit_include_directories: true,
    dependencies: deps,
    link_with: [ libmanager ],
    install: true,
    install_dir: get_option('bindir'),
    include_directories: ['./', '../', './gen'],
)
