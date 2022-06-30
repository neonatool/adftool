SUBDIRS = lib po
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = m4/gnulib-cache.m4

AM_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I lib -I $(srcdir)/lib \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"$(LOCALEDIR)\"

AM_DISTCHECK_CONFIGURE_FLAGS = \
  --disable-valgrind-tests \
  "CFLAGS=-g -O3 -fsanitize=address -fsanitize=undefined -Wall -Wextra" \
  SHELL=$(SHELL)

lib_LTLIBRARIES = libadftool.la
dist_include_HEADERS = include/adftool.h
check_PROGRAMS = bplus_tree_lookup_example

bplus_tree_lookup_example_SOURCES = \
  src/bplus_tree_lookup_example.c
bplus_tree_lookup_example_LDADD = \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@

TESTS = $(check_PROGRAMS)
LOG_COMPILER = $(LOG_VALGRIND)

libadftool_la_SOURCES = \
  src/adftool_private.h \
  src/libadftool/lookup.c \
  src/libadftool/parameters.c \
  src/libadftool/bplus_key.c \
  src/libadftool/version.c \
  src/libadftool/emscripten.cpp
libadftool_la_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I lib -I $(srcdir)/lib \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"$(LOCALEDIR)\" \
  -DBUILDING_LIBADFTOOL
libadftool_la_CFLAGS = \
    $(CFLAG_VISIBILITY)
libadftool_la_LIBADD = lib/libadftool.la @LTLIBOBJS@
libadftool_la_LDFLAGS = -no-undefined $(LTLIBINTL)

if WITH_HDF5
  libadftool_la_SOURCES += \
    src/libadftool/hdf5.c
else # not WITH_HDF5
  libadftool_la_SOURCES += \
    src/libadftool/not-hdf5.c
  AM_CPPFLAGS += \
    -DLIBADFTOOL_WITHOUT_HDF5
  libadftool_la_CPPFLAGS += \
    -DLIBADFTOOL_WITHOUT_HDF5
endif # not WITH_HDF5
