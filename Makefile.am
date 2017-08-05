BUILT_SOURCES=generated.cpp extra_ifaces.cpp gen_serialization.hpp

CLEANFILES=$(BUILT_SOURCES)

sbin_PROGRAMS = phosphor-inventory
noinst_LTLIBRARIES = libmanagercommon.la libmanager.la

extra_yamldir=$(YAML_PATH)/extra_interfaces.d

phosphor_inventory_SOURCES = app.cpp
phosphor_inventory_LDADD = libmanager.la $(SDBUSPLUS_LIBS)
phosphor_inventory_CXXFLAGS = $(SDBUSPLUS_CFLAGS)

libmanagercommon_la_LDFLAGS = -static
libmanagercommon_la_SOURCES = \
	errors.cpp \
	functor.cpp \
	manager.cpp
libmanagercommon_la_LIBADD = \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	-lstdc++fs
libmanagercommon_la_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

libmanager_la_LDFLAGS = -static
libmanager_la_SOURCES = \
	extra_ifaces.cpp \
	generated.cpp
libmanager_la_LIBADD = \
	libmanagercommon.la \
	$(SDBUSPLUS_LIBS)

clean-local: clean-extra

if IFACES_PATH
PIMGEN_ARGS=-i $(IFACES_PATH)
endif

generated.cpp:
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/pimgen.py $(PIMGEN_ARGS) -d $(extra_yamldir)/.. -o $(builddir) generate-cpp

gen_serialization.hpp:
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/pimgen.py $(PIMGEN_ARGS) -d $(extra_yamldir)/.. -o $(builddir) generate-serialization

SUBDIRS = . test

-include Makefile.extra
