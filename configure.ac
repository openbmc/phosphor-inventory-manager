# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-inventory-manager], [1.0], [https://github.com/openbmc/phosphor-inventory-manager/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_CHECK_PROG([FIND], find, find)
AC_CHECK_PROG([BASENAME], basename, basename)
AC_CHECK_PROG([DIRNAME], dirname, dirname)
AM_PATH_PYTHON([2.7],
    [AC_SUBST([PYTHON], [echo "$PYTHON"])],
    [AC_MSG_ERROR([Could not find python-2.7 installed...python-2.7 is required])])

# Checks for libraries.
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus],,
    AC_MSG_ERROR(["Requires sdbusplus package."]))

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_14([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

# Checks for header files.
AC_CHECK_HEADER(systemd/sd-bus.h, ,[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd developement package required])])
AC_CHECK_HEADER(sdbusplus/server.hpp, ,[AC_MSG_ERROR([Could not find sdbusplus/server.hpp...sdbusplus developement package required])])

# Checks for library functions.
LT_INIT # Removes 'unrecognized options: --with-libtool-sysroot'
# Check/set gtest specific functions.
AX_PTHREAD([GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"],[GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=0"])
AC_SUBST(GTEST_CPPFLAGS)

AC_ARG_ENABLE([oe-sdk],
    AS_HELP_STRING([--enable-oe-sdk], [Link testcases absolutely against OE SDK so they can be ran within it.])
)
AC_ARG_VAR(OECORE_TARGET_SYSROOT,
    [Path to the OE SDK SYSROOT])
AS_IF([test "x$enable_oe_sdk" == "xyes"],
    AS_IF([test "x$OECORE_TARGET_SYSROOT" == "x"],
          AC_MSG_ERROR([OECORE_TARGET_SYSROOT must be set with --enable-oe-sdk])
    )
    AC_MSG_NOTICE([Enabling OE-SDK at $OECORE_TARGET_SYSROOT])
    [
        testcase_flags="-Wl,-rpath,\${OECORE_TARGET_SYSROOT}/lib"
        testcase_flags="${testcase_flags} -Wl,-rpath,\${OECORE_TARGET_SYSROOT}/usr/lib"
        testcase_flags="${testcase_flags} -Wl,-dynamic-linker,`find \${OECORE_TARGET_SYSROOT}/lib/ld-*.so | sort -r -n | head -n1`"
    ]
    AC_SUBST([OESDK_TESTCASE_FLAGS], [$testcase_flags])
)

AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])
AS_IF([test "x$SDBUSPLUSPLUS" == "x"],
    AC_MSG_ERROR([Cannot find sdbus++]))

AC_ARG_VAR(BUSNAME, [The DBus busname to own.])
AC_ARG_VAR(INVENTORY_ROOT, [The DBus inventory namespace root.])
AC_ARG_VAR(IFACE, [The manager DBus interface.])
AC_ARG_VAR(YAML_PATH, [The path to the yaml config files.])
AS_IF([test "x$BUSNAME" == "x"], [BUSNAME="xyz.openbmc_project.Inventory.Manager"])
AS_IF([test "x$INVENTORY_ROOT" == "x"], [INVENTORY_ROOT="/xyz/openbmc_project/Inventory"])
AS_IF([test "x$IFACE" == "x"], [IFACE="xyz.openbmc_project.Inventory.Manager"])
AS_IF([test "x$YAML_PATH" == "x"], [YAML_PATH="$srcdir/example"])
AC_DEFINE_UNQUOTED([BUSNAME], ["$BUSNAME"], [The DBus busname to own.])
AC_DEFINE_UNQUOTED([INVENTORY_ROOT], ["$INVENTORY_ROOT"], [The DBus inventory namespace root.])
AC_DEFINE_UNQUOTED([IFACE], ["$IFACE"], [The manager DBus interface.])

# Create configured output
AC_CONFIG_FILES([Makefile.extra],
    [${srcdir}/generate_makefile.sh $yaml > Makefile.extra],
    [yaml=${YAML_PATH}/extra_interfaces.d])
AC_CONFIG_FILES([test/Makefile.extra],
    [${srcdir}/generate_makefile.sh $test_yaml > test/Makefile.extra],
    [test_yaml=$srcdir/example/extra_interfaces.d])
AC_CONFIG_FILES([Makefile test/Makefile])
AC_OUTPUT
