SUBDIRS = lib libprog po
ACLOCAL_AMFLAGS = -I m4 -I m4prog
EXTRA_DIST = m4/gnulib-cache.m4 m4prog/gnulib-cache.m4

AM_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I lib -I $(srcdir)/lib \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"$(LOCALEDIR)\"

AM_DISTCHECK_CONFIGURE_FLAGS = \
  --disable-valgrind-tests \
  "CFLAGS=-g -O3 -fsanitize=address -fsanitize=undefined -Wall -Wextra" \
  SHELL=$(SHELL)

RELOCATABLE_LIBRARY_PATH = $(libdir)

bin_PROGRAMS = adftool

lib_LTLIBRARIES = libadftool.la
dist_include_HEADERS = include/adftool.h
check_PROGRAMS = bplus_tree_lookup_example

TESTS = bplus_tree_lookup_example
LOG_COMPILER = $(LOG_VALGRIND)

adftool_SOURCES = src/adftool.c
adftool_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"$(LOCALEDIR)\" \
  -DINSTALLDIR=\"$(bindir)\"
adftool_LDADD = libprog/libgnu.la @LTLIBOBJS@ $(LTLIBINTL)
adftool_LDFLAGS =

libadftool_la_SOURCES = \
  src/adftool_private.h \
  src/libadftool/lookup.c \
  src/libadftool/parameters.c \
  src/libadftool/bplus_key.c \
  src/libadftool/hdf5.c \
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

if RELOCATABLE_VIA_LD
libadftool_la_LDFLAGS += `$(RELOCATABLE_LDFLAGS) $(libdir)`
adftool_LDFLAGS += `$(RELOCATABLE_LDFLAGS) $(bindir)`
endif

bplus_tree_lookup_example_SOURCES = \
  src/bplus_tree_lookup_example.c
bplus_tree_lookup_example_LDADD = \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@
