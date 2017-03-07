BUILT_SOURCES=generated.cpp extra_ifaces.cpp
CLEANFILES=$(BUILT_SOURCES)

AM_CPPFLAGS = -iquote$(top_srcdir)

check_PROGRAMS =
noinst_PROGRAMS = phosphor-inventory-test
noinst_LTLIBRARIES = libtest.la

extra_yamldir=$(top_srcdir)/example/extra_interfaces.d

phosphor_inventory_test_SOURCES = test.cpp
phosphor_inventory_test_LDADD = \
	libtest.la
	$(SDBSUPLUS_LIBS)
phosphor_inventory_test_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PTHREAD_CFLAGS)

libtest_la_LDFLAGS = -static
libtest_la_SOURCES = \
	extra_ifaces.cpp \
	generated.cpp
libtest_la_LIBADD = \
	$(top_builddir)/libmanagercommon.la \
	$(SDBUSPLUS_LIBS)

clean-local: clean-extra

generated.cpp:
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/pimgen.py -d $(extra_yamldir)/.. -o $(builddir) generate-cpp

-include Makefile.extra
