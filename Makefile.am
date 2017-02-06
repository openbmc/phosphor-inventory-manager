BUILT_SOURCES=generated.cpp extra_ifaces.cpp

CLEANFILES=$(BUILT_SOURCES)

sbin_PROGRAMS = phosphor-inventory
noinst_LTLIBRARIES = libmanagercommon.la libmanager.la

extra_yamldir=$(YAML_PATH)/extra_interfaces.d

phosphor_inventory_SOURCES = app.cpp
phosphor_inventory_LDADD = libmanager.la
phosphor_inventory_LDFLAGS = $(SDBUSPLUS_LIBS)
phosphor_inventory_CFLAGS = $(SDBUSPLUS_CFLAGS)

libmanagercommon_la_SOURCES = \
	errors.cpp \
	functor.cpp \
	manager.cpp
libmanagercommon_la_LIBADD = $(SDBUSPLUS_LIBS) $(PHOSPHOR_DBUS_INTERFACES_LIBS)
libmanagercommon_la_CFLAGS = $(SDBUSPLUS_CFLAGS) $(PHOSPHOR_DBUS_INTERACES_CFLAGS)

libmanager_la_SOURCES = \
	extra_ifaces.cpp \
	generated.cpp
libmanager_la_LIBADD = libmanagercommon.la $(SDBUSPLUS_LIBS)

clean-local: clean-extra

generated.cpp:
	$(AM_V_GEN)$(top_srcdir)/pimgen.py -d $(extra_yamldir)/.. -o $(builddir) generate-cpp

SUBDIRS = . test

-include Makefile.extra
