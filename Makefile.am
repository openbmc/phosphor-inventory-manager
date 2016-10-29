sbin_PROGRAMS = phosphor-inventory

phosphor_inventory_SOURCES = \
	app.cpp \
	xyz.openbmc_project.Inventory.Manager.cpp \
	filters.cpp \
	manager.cpp
phosphor_inventory_LDFLAGS = $(SYSTEMD_LIBS)
phosphor_inventory_CFLAGS = $(SYSTEMD_CFLAGS)

SUBDIRS = test
