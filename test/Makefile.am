check_PROGRAMS =
noinst_PROGRAMS = phosphor-inventory-test

phosphor_inventory_test_SOURCES = \
	test.cpp
phosphor_inventory_test_LDFLAGS = $(SYSTEMD_LIBS)
phosphor_inventory_test_CFLAGS = $(SYSTEMD_CFLAGS)
phosphor_inventory_test_LDADD = ${top_builddir}/manager.o \
				${top_builddir}/filters.o \
				${top_builddir}/generated.o \
				${top_builddir}/xyz.openbmc_project.Inventory.Manager.o
if EXAMPLE
phosphor_inventory_test_LDADD += ${top_builddir}/libexample.la
endif
