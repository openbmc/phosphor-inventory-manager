gtest_dep = dependency('gtest', main: true, disabler: true, required: false)
gmock_dep = dependency('gmock', disabler: true, required: false)
if not gtest_dep.found() or not gmock_dep.found()
    gtest_proj = import('cmake').subproject('googletest', required: false)
    if gtest_proj.found()
        gtest_dep = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest'),
                gtest_proj.dependency('gtest_main'),
            ],
        )
        gmock_dep = gtest_proj.dependency('gmock')
    else
        assert(
            not get_option('tests').enabled(),
            'Googletest is required if tests are enabled',
        )
    endif
endif

test_sources = [
    generated_cpp,
    gen_serialization_hpp,
    '../association_manager.cpp',
    '../manager.cpp',
    '../functor.cpp',
    '../errors.cpp',
]

tests = [
    'associations_test.cpp',
    'interface_ops_test.cpp',
    'manager_test.cpp',
    'serialize_test.cpp',
    'types_test.cpp',
    'utils_test.cpp',
]

test_deps = [
    sdbusplus_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    nlohmann_json_dep,
    cereal_dep,
]

foreach t : tests
    test(
        t,
        executable(
            t.underscorify(),
            t,
            test_sources,
            include_directories: ['..'],
            dependencies: [gtest_dep, gmock_dep, test_deps],
        ),
        workdir: meson.current_source_dir(),
    )
endforeach
