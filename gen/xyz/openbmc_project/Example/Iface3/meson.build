# Generated file; do not modify.
generated_sources += custom_target(
    'xyz/openbmc_project/Example/Iface3__cpp'.underscorify(),
    input: [ '../../../../../example/extra_interfaces.d/xyz/openbmc_project/Example/Iface3.interface.yaml',  ],
    output: [ 'server.cpp', 'server.hpp', 'client.hpp',  ],
    command: [
        sdbuspp_gen_meson_prog, '--command', 'cpp',
        '--output', meson.current_build_dir(),
        '--tool', sdbusplusplus_prog,
        '--directory', meson.current_source_dir() / '../../../../../example/extra_interfaces.d',
        'xyz/openbmc_project/Example/Iface3',
    ],
)

