sbin_PROGRAMS = phosphor-inventory

phosphor_inventory_SOURCES = \
	app.cpp \
	xyz.openbmc_project.Inventory.Manager.cpp \
	filters.cpp \
	generated.cpp \
	manager.cpp

if EXAMPLE
noinst_LTLIBRARIES = libexample.la
libexample_la_SOURCES = \
	xyz.openbmc_project.Example.Iface1.cpp	\
	xyz.openbmc_project.Example.Iface2.cpp
phosphor_inventory_LDADD = libexample.la
endif
phosphor_inventory_LDFLAGS = $(SYSTEMD_LIBS)
phosphor_inventory_CFLAGS = $(SYSTEMD_CFLAGS)

SUBDIRS = . test
