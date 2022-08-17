SUBDIRS = lib libprog . po
ACLOCAL_AMFLAGS = -I m4 -I m4prog
EXTRA_DIST = m4/gnulib-cache.m4 m4prog/gnulib-cache.m4

CLEANFILES = \
  bplus-tree-grow-file-example.hdf5 \
  bplus-tree-gets-initialized.hdf5 \
  corrupted.adf \
  well-formed.adf \
  non-existing.adf \
  dictionary_example.adf \
  term_example.adf \
  statement_example.adf \
  multi_statement_example.adf \
  channel_metadata_example.adf \
  eeg_data_example.adf

AM_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I lib -I $(srcdir)/lib \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\"

AM_DISTCHECK_CONFIGURE_FLAGS = \
  --disable-valgrind-tests \
  "CFLAGS=-g3 -O2 -DFORTIFY_SOURCE=2 -fsanitize=undefined -fsanitize-undefined-trap-on-error -Wall -Wextra" \
  SHELL=$(SHELL)

RELOCATABLE_LIBRARY_PATH = $(libdir)

bin_PROGRAMS = adftool

lib_LTLIBRARIES = libadftool.la
dist_include_HEADERS = include/adftool.h
check_PROGRAMS = \
  bplus_tree_lookup_example \
  bplus_tree_empty_lookup_example \
  bplus_tree_grow_example \
  bplus_tree_grow_file_example \
  bplus_tree_gets_initialized \
  bplus_tree_insert_example \
  file_open_example \
  dictionary_example \
  term_manipulation_example \
  statement_manipulation_example \
  multi_statement_example \
  term_parser_example \
  channel_metadata_example \
  eeg_data_example

TESTS = \
  bplus_tree_lookup_example \
  bplus_tree_empty_lookup_example \
  bplus_tree_grow_example \
  bplus_tree_grow_file_example \
  bplus_tree_gets_initialized \
  bplus_tree_insert_example \
  file_open_example \
  dictionary_example \
  term_manipulation_example \
  statement_manipulation_example \
  multi_statement_example \
  term_parser_example \
  channel_metadata_example \
  eeg_data_example

LOG_COMPILER = $(LOG_VALGRIND)

adftool_SOURCES = src/adftool.c
adftool_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"
adftool_LDADD = libprog/libgnu.la @LTLIBOBJS@ $(LTLIBINTL) libadftool.la
adftool_LDFLAGS =

libadftool_la_SOURCES = \
  src/adftool_private.h \
  src/adftool_bplus_node.h \
  src/adftool_bplus_key.h \
  src/adftool_bplus.h \
  src/adftool_hdf5.h \
  src/adftool_grow.h \
  src/adftool_lookup.h \
  src/adftool_insert.h \
  src/libadftool/version.c \
  src/libadftool/file.c \
  src/libadftool/dictionary.c \
  src/libadftool/term.c \
  src/libadftool/statement.c \
  src/libadftool/results.c \
  src/libadftool/indices.c \
  src/libadftool/lexer.l \
  src/libadftool/channel_metadata.c \
  src/libadftool/eeg_data.c \
  src/libadftool/emscripten.cpp
libadftool_la_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I lib -I $(srcdir)/lib \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DBUILDING_LIBADFTOOL
libadftool_la_CFLAGS = \
    $(CFLAG_VISIBILITY)
libadftool_la_LFLAGS = --prefix=_adftool_lexer_
libadftool_la_LIBADD = lib/libadftool.la @LTLIBOBJS@
libadftool_la_LDFLAGS = -no-undefined $(LTLIBINTL)

if RELOCATABLE_VIA_LD
libadftool_la_LDFLAGS += `$(RELOCATABLE_LDFLAGS) $(libdir)`
adftool_LDFLAGS += `$(RELOCATABLE_LDFLAGS) $(bindir)`
endif

bplus_tree_lookup_example_SOURCES = \
  src/bplus_tree_lookup_example.c
bplus_tree_lookup_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@

bplus_tree_empty_lookup_example_SOURCES = \
  src/bplus_tree_empty_lookup_example.c
bplus_tree_empty_lookup_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@

bplus_tree_grow_example_SOURCES = \
  src/bplus_tree_grow_example.c
bplus_tree_grow_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@

bplus_tree_grow_file_example_SOURCES = \
  src/bplus_tree_grow_file_example.c
bplus_tree_grow_file_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@

bplus_tree_gets_initialized_SOURCES = \
  src/bplus_tree_gets_initialized.c
bplus_tree_gets_initialized_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@

bplus_tree_insert_example_SOURCES = \
  src/bplus_tree_insert_example.c
bplus_tree_insert_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@

file_open_example_SOURCES = \
  src/file_open_example.c
file_open_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@
file_open_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"

dictionary_example_SOURCES = \
  src/dictionary_example.c
dictionary_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@
dictionary_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"

term_manipulation_example_SOURCES = \
  src/term_manipulation_example.c
term_manipulation_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@
term_manipulation_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"

statement_manipulation_example_SOURCES = \
  src/statement_manipulation_example.c
statement_manipulation_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@
statement_manipulation_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"

multi_statement_example_SOURCES = \
  src/multi_statement_example.c
multi_statement_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@
multi_statement_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"

term_parser_example_SOURCES = \
  src/term_parser_example.c
term_parser_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@
term_parser_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"

channel_metadata_example_SOURCES = \
  src/channel_metadata_example.c
channel_metadata_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@
channel_metadata_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"

eeg_data_example_SOURCES = \
  src/eeg_data_example.c
eeg_data_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  @LTLIBOBJS@
eeg_data_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"
