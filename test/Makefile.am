check_PROGRAMS =
noinst_PROGRAMS = phosphor-inventory-test

phosphor_inventory_test_SOURCES = \
	test.cpp
phosphor_inventory_test_LDFLAGS = $(SYSTEMD_LIBS) $(PTHREAD_CFLAGS)
phosphor_inventory_test_CFLAGS = $(SYSTEMD_CFLAGS)
phosphor_inventory_test_LDADD = ${top_builddir}/libmanager.la
