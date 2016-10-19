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

# Checks for libraries.
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])

# Checks for header files.
AC_CHECK_HEADER(systemd/sd-bus.h, ,[AC_MSG_ERROR([Could not find systemd/sd-bus.h...systemd developement package required])])
AC_CHECK_HEADER(sdbusplus/server.hpp, ,[AC_MSG_ERROR([Could not find sdbusplus/server.hpp...sdbusplus developement package required])])

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_14([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

# Checks for library functions.
LT_INIT # Removes 'unrecognized options: --with-libtool-sysroot'

AC_ARG_VAR(BUSNAME, [The DBus busname to own.])
AC_ARG_VAR(INVENTORY_ROOT, [The DBus inventory namespace root.])
AC_ARG_VAR(IFACE, [The manager DBus interface.])
AS_IF([test "x$BUSNAME" == "x"], [BUSNAME="xyz.openbmc_project.Inventory.Manager"])
AS_IF([test "x$INVENTORY_ROOT" == "x"], [INVENTORY_ROOT="/xyz/openbmc_project/Inventory"])
AS_IF([test "x$IFACE" == "x"], [IFACE="xyz.openbmc_project.Inventory.Manager"])
AC_DEFINE_UNQUOTED([BUSNAME], ["$BUSNAME"], [The DBus busname to own.])
AC_DEFINE_UNQUOTED([INVENTORY_ROOT], ["$INVENTORY_ROOT"], [The DBus inventory namespace root.])
AC_DEFINE_UNQUOTED([IFACE], ["$IFACE"], [The manager DBus interface.])

# Create configured output
AC_CONFIG_FILES([Makefile test/Makefile])
AC_OUTPUT
