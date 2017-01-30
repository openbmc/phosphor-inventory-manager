EXTRAIFACES_C=@EXTRAIFACES_C@
EXTRAIFACES_LO=@EXTRAIFACES_LO@
BUILT_SOURCES=generated.cpp

CLEANFILES=$(BUILT_SOURCES) $(EXTRAIFACES_C) libextra.la

sbin_PROGRAMS = phosphor-inventory
phosphor_inventory_SOURCES = app.cpp
phosphor_inventory_LDADD = libmanager.la
phosphor_inventory_LDFLAGS = $(SYSTEMD_LIBS)
phosphor_inventory_CFLAGS = $(SYSTEMD_CFLAGS)

noinst_LTLIBRARIES = libmanager.la
libmanager_la_SOURCES = \
	xyz.openbmc_project.Inventory.Manager.cpp \
	generated.cpp \
	errors.cpp \
	functor.cpp \
	manager.cpp
libmanager_la_LIBADD = libextra.la

# Automake does not allow autoconf substituted variables
# in _SOURCES variables.  Work around this by providing
# our own target.
#
# Typically the recommendation is conditional compilation
# but here the generated source files are completely
# arbitrary and not known making that approach difficult.
libextra.la: $(EXTRAIFACES_LO)
	$(AM_V_CXXLD)$(CXXLINK) $(EXTRAIFACES_LO)

$(EXTRAIFACES_C) generated.cpp:
	$(AM_V_GEN)@PIMGEN@ -o $(top_builddir) generate-cpp

SUBDIRS = . test
