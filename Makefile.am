SUBDIRS = lib libprog . po tests
ACLOCAL_AMFLAGS = -I m4 -I m4prog
EXTRA_DIST = m4/gnulib-cache.m4 m4prog/gnulib-cache.m4 \
  src/filter_test_data.c src/filter_test_data.R \
  po/Rules-install-local \
  conanfile-in.py \
  $(top_srcdir)/conanfile.py \
  $(top_srcdir)/.version \
  $(top_srcdir)/package.json.in

info_TEXINFOS = doc/adftool.texi

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
  eeg_data_example.adf \
  eeg_metadata_example.adf

AM_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I lib -I $(srcdir)/lib \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\"

AM_DISTCHECK_CONFIGURE_FLAGS = \
  --disable-valgrind-tests \
  "CFLAGS=-g3 -O2 -DFORTIFY_SOURCE=2 -fsanitize=undefined,address -fsanitize-undefined-trap-on-error -Wall -Wextra" \
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
  eeg_data_example \
  filter_example \
  data_file_example \
  date_parser_example \
  eeg_metadata_example

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
  eeg_data_example \
  filter_example \
  data_file_example \
  date_parser_example \
  eeg_metadata_example

LOG_COMPILER = $(LOG_VALGRIND)

adftool_SOURCES = src/adftool.c
adftool_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"
adftool_LDADD = \
  libprog/libgnu.la \
  @LTLIBOBJS@ \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  libadftool.la
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
  src/libadftool/indices.c \
  src/libadftool/lexer.l \
  src/libadftool/channel_metadata.c \
  src/libadftool/eeg_data.c \
  src/libadftool/eeg_metadata.c \
  src/libadftool/fir.c \
  src/libadftool/array.c
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
libadftool_la_LDFLAGS = \
  -no-undefined \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  $(LIB_GETRANDOM) \
  $(MODF_LIBM) \
  $(TRUNC_LIBM)

if PYTHON_MODULE
pyexec_LTLIBRARIES = adftool.la
adftool_la_SOURCES = \
  src/adftool_python.c
adftool_la_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I lib -I $(srcdir)/lib \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DBUILDING_LIBADFTOOL_PYTHON
adftool_la_CFLAGS = $(CFLAG_VISIBILITY)
adftool_la_LIBADD = libadftool.la lib/libadftool.la @LTLIBOBJS@
adftool_la_LDFLAGS = \
  -no-undefined -avoid-version -module \
  $(LTLIBINTL)
endif

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
  $(LIB_CLOCK_GETTIME) \
  @LTLIBOBJS@

bplus_tree_empty_lookup_example_SOURCES = \
  src/bplus_tree_empty_lookup_example.c
bplus_tree_empty_lookup_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  @LTLIBOBJS@

bplus_tree_grow_example_SOURCES = \
  src/bplus_tree_grow_example.c
bplus_tree_grow_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  @LTLIBOBJS@

bplus_tree_grow_file_example_SOURCES = \
  src/bplus_tree_grow_file_example.c
bplus_tree_grow_file_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  @LTLIBOBJS@

bplus_tree_gets_initialized_SOURCES = \
  src/bplus_tree_gets_initialized.c
bplus_tree_gets_initialized_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  @LTLIBOBJS@

bplus_tree_insert_example_SOURCES = \
  src/bplus_tree_insert_example.c
bplus_tree_insert_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  @LTLIBOBJS@

file_open_example_SOURCES = \
  src/file_open_example.c
file_open_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
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
  $(LIB_CLOCK_GETTIME) \
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
  $(LIB_CLOCK_GETTIME) \
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
  $(LIB_CLOCK_GETTIME) \
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
  $(LIB_CLOCK_GETTIME) \
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
  $(LIB_CLOCK_GETTIME) \
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
  $(LIB_CLOCK_GETTIME) \
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
  $(LIB_CLOCK_GETTIME) \
  @LTLIBOBJS@
eeg_data_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"

filter_example_SOURCES = \
  src/filter_example.c src/filter_test_data.c
filter_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  @LTLIBOBJS@
filter_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"

data_file_example_SOURCES = \
  src/data_file_example.c
data_file_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  @LTLIBOBJS@
data_file_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"

date_parser_example_SOURCES = \
  src/date_parser_example.c
date_parser_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  @LTLIBOBJS@
date_parser_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"

eeg_metadata_example_SOURCES = \
  src/eeg_metadata_example.c
eeg_metadata_example_LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  @LTLIBOBJS@
eeg_metadata_example_CPPFLAGS = \
  -I include -I $(srcdir)/include \
  -I libprog -I $(srcdir)/libprog \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"@localedir@\" \
  -DINSTALLDIR=\"$(bindir)\"

AM_TESTS_ENVIRONMENT = \
  mkdir -p $(abs_top_builddir)/.tmp ; \
  export TMPDIR=$(abs_top_builddir)/.tmp ;

BUILT_SOURCES = src/filter_test_data.c

$(srcdir)/src/filter_test_data.c: src/filter_test_data.R
	$(AM_V_GEN) $(RSCRIPT) $< > $@-t
	@mv $@-t $@

$(top_srcdir)/.version:
	$(AM_V_GEN) echo '$(VERSION)' > $@-t
	@mv $@-t $@

$(top_srcdir)/conanfile.py: conanfile-in.py
	$(AM_V_GEN) $(SED) 's/\@VERSION\@/$(VERSION)/g' $< > $@-t
	@mv $@-t $@

$(top_srcdir)/package.json: package.json.in
	$(AM_V_GEN) $(SED) "s/\\@VERSION\\@/$$(echo $(VERSION) | cut -d '.' -f 1-3)/g" $< > $@-t
	@mv $@-t $@

dist-hook:
	echo '$(VERSION)' > $(distdir)/.tarball-version

if NODE_MODULE
index.mjs: libadftool.la
	mkdir -p js-output
	(cd js-output ; ../libtool --tag=CC --mode=link $(CC) -O3 ../$< -sEXPORT_ALL=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString -sEXPORTED_FUNCTIONS=_malloc,_free,_adftool_file_alloc,_adftool_file_free,_adftool_file_open,_adftool_file_open,_adftool_file_close,_adftool_file_open_data,_adftool_file_get_data,_adftool_dictionary_get,_adftool_dictionary_lookup,_adftool_dictionary_insert,_adftool_term_alloc,_adftool_term_free,_adftool_term_copy,_adftool_term_set_blank,_adftool_term_set_named,_adftool_term_set_literal,_adftool_term_set_integer,_adftool_term_set_double,_adftool_term_set_date,_adftool_term_is_blank,_adftool_term_is_named,_adftool_term_is_literal,_adftool_term_is_typed_literal,_adftool_term_is_langstring,_adftool_term_value,_adftool_term_meta,_adftool_term_as_integer,_adftool_term_as_double,_adftool_term_as_date,_adftool_term_compare,_adftool_term_decode,_adftool_term_encode,_adftool_term_parse_n3,_adftool_term_to_n3,_adftool_statement_alloc,_adftool_statement_free,_adftool_statement_set,_adftool_statement_get,_adftool_statement_copy,_adftool_statement_compare,_adftool_quads_get,_adftool_quads_delete,_adftool_quads_insert,_adftool_lookup,_adftool_lookup_objects,_adftool_lookup_subjects,_adftool_delete,_adftool_insert,_adftool_find_channel_identifier,_adftool_get_channel_column,_adftool_add_channel_type,_adftool_get_channel_types,_adftool_find_channels_by_type,_adftool_get_channel_decoder,_adftool_set_channel_decoder,_adftool_eeg_set_data,_adftool_eeg_get_data,_adftool_eeg_get_time,_adftool_eeg_set_time,_adftool_fir_alloc,_adftool_fir_free,_adftool_fir_design_bandpass,_adftool_fir_apply,_adftool_array_get_byte,_adftool_array_set_byte,_adftool_sizeof_size_t,_adftool_array_get_size_t,_adftool_array_set_size_t,_adftool_sizeof_timespec,_adftool_array_get_tv_sec,_adftool_array_get_tv_nsec,_adftool_array_set_timespec,_adftool_sizeof_pointer,_adftool_array_get_pointer,_adftool_array_set_pointer,_adftool_array_get_uint64,_adftool_array_set_uint64,_adftool_sizeof_double,_adftool_array_get_double,_adftool_array_set_double $(CFLAGS) $(LDFLAGS) $(LIBS) -o index.mjs -sSINGLE_FILE -sBINARYEN_ASYNC_COMPILATION=0 && cp index.mjs $(abs_top_builddir)/index.mjs)
	rm -rf js-output

nodejsdir = $(datadir)/nodejs
pkgnodejsdir = $(nodejsdir)/$(PACKAGE)
pkgnodejs_DATA = index.mjs package.json
endif
