sbin_PROGRAMS = phosphor-inventory

phosphor_inventory_SOURCES = \
	app.cpp \
	server.cpp \
	filters.cpp \
	actions.cpp \
	manager.cpp
phosphor_inventory_LDFLAGS = $(SYSTEMD_LIBS)
phosphor_inventory_CFLAGS = $(SYSTEMD_CFLAGS)

SUBDIRS = test
