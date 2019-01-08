BUILT_SOURCES=generated.cpp extra_ifaces.cpp gen_serialization.hpp
CLEANFILES=$(BUILT_SOURCES)

AM_CPPFLAGS = -iquote$(top_srcdir)

check_PROGRAMS =
noinst_PROGRAMS = phosphor-inventory-test
noinst_LTLIBRARIES = libtest.la

TESTS = ${check_PROGRAMS}

types_test_SOURCES = types_test.cpp
types_test_CFLAGS = ${GTEST_CFLAGS} ${GMOCK_CFLAGS}
types_test_LDADD = ${GTEST_LIBS} ${GMOCK_LIBS}
types_test_LDFLAGS = ${OESDK_TESTCASE_FLAGS}
check_PROGRAMS += types-test

utils_test_SOURCES = utils_test.cpp
utils_test_CFLAGS = ${GTEST_CFLAGS} ${GMOCK_CFLAGS}
utils_test_LDADD = ${GTEST_LIBS} ${GMOCK_LIBS}
utils_test_LDFLAGS = ${OESDK_TESTCASE_FLAGS}
check_PROGRAMS += utils-test

extra_yamldir=$(top_srcdir)/example/extra_interfaces.d

phosphor_inventory_test_SOURCES = test.cpp
phosphor_inventory_test_LDADD = \
	libtest.la
	$(SDBSUPLUS_LIBS) \
	$(PTHREAD_LIBS)
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

gen_serialization.hpp:
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/pimgen.py -d $(extra_yamldir)/.. -o $(builddir) generate-serialization

-include Makefile.extra
