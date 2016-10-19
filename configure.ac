# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-inventory-manager], [1.0], [https://github.com/openbmc/phosphor-inventory-manager/issues])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_14([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

# Checks for library functions.
LT_INIT # Removes 'unrecognized options: --with-libtool-sysroot'

# Create configured output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
