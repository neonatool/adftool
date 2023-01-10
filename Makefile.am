SUBDIRS = lib libprog . po tests
ACLOCAL_AMFLAGS = -I m4 -I m4prog
EXTRA_DIST = m4/gnulib-cache.m4 m4prog/gnulib-cache.m4 \
  src/filter_test_data.c src/filter_test_data.R src/filter_test_data.csv \
  po/Rules-install-local \
  $(top_srcdir)/.version \
  $(top_srcdir)/src/js/package.json.in

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
  eeg_metadata_example.adf \
  filter-log

COMMON_COMPILE_FLAGS = \
  -I include -I $(srcdir)/include \
  -I src -I $(srcdir)/src \
  -DLOCALEDIR=\"$(localedir)\" \
  -DINSTALLDIR=\"$(bindir)\"

COMMON_LINK_FLAGS = \
  $(LTLIBOBJS) \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME)

AM_CPPFLAGS = \
  -I libprog -I $(srcdir)/libprog \
  $(COMMON_COMPILE_FLAGS)

LDADD = \
  libprog/libgnu.la \
  libadftool.la \
  $(COMMON_LINK_FLAGS) \
  $(LIBTHREAD) \
  $(LIB_SETLOCALE) \
  $(LIB_SETLOCALE_NULL) \
  $(INTL_MACOSX_LIBS)

AM_DISTCHECK_CONFIGURE_FLAGS = \
  --disable-valgrind-tests \
  "CFLAGS=-g3 -O2 -DFORTIFY_SOURCE=2 -fsanitize=undefined,address -fsanitize-undefined-trap-on-error -Wall -Wextra -fanalyzer" \
  SHELL=$(SHELL)

RELOCATABLE_LIBRARY_PATH = $(libdir)

bin_PROGRAMS = src/adftool

lib_LTLIBRARIES = libadftool.la
dist_include_HEADERS = include/adftool.h
check_PROGRAMS = \
  src/file_open_example \
  src/dictionary_example \
  src/term_manipulation_example \
  src/statement_manipulation_example \
  src/multi_statement_example \
  src/term_parser_example \
  src/channel_metadata_example \
  src/eeg_data_example \
  src/filter_example \
  src/data_file_example \
  src/date_parser_example \
  src/eeg_metadata_example \
  src/timespec_example \
  src/statement_get_with_arrays \
  src/check_fine_eeg_get_data \
  src/check_lookup_pagination \
  src/simple_filters

TESTS = $(check_PROGRAMS)

LOG_COMPILER = $(LOG_VALGRIND)

src_adftool_LDADD = \
  libprog/libgnu.la \
  $(LTLIBOBJS) \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  libadftool.la

libadftool_la_SOURCES = \
  src/libadftool/array.c \
  src/libadftool/channel_metadata.c \
  src/libadftool/dictionary_bytes.h \
  src/libadftool/dictionary_cache.h \
  src/libadftool/dictionary_index.h \
  src/libadftool/dictionary_strings.h \
  src/libadftool/eeg_data.c \
  src/libadftool/eeg_metadata.c \
  src/libadftool/file.c \
  src/libadftool/file.h \
  src/libadftool/fir.c \
  src/libadftool/indices.c \
  src/libadftool/lexer.l \
  src/libadftool/quads.h \
  src/libadftool/quads_index.h \
  src/libadftool/statement.c \
  src/libadftool/statement.h \
  src/libadftool/term.c \
  src/libadftool/term.h \
  src/libadftool/version.c
libadftool_la_CPPFLAGS = \
  -I lib -I $(srcdir)/lib \
  -DBUILDING_LIBADFTOOL \
  $(COMMON_COMPILE_FLAGS)
libadftool_la_CFLAGS = \
    $(CFLAG_VISIBILITY)
libadftool_la_LFLAGS = --prefix=_adftool_lexer_
libadftool_la_LIBADD = lib/libadftool.la $(LTLIBOBJS)
libadftool_la_LDFLAGS = \
  -no-undefined \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  $(LIB_GETRANDOM) \
  $(MODF_LIBM) \
  $(TRUNC_LIBM)

adftool_la_LDFLAGS =
if PYTHON_MODULE
pyexec_LTLIBRARIES = adftool.la
adftool_la_SOURCES = \
  src/adftool_python.c
adftool_la_CPPFLAGS = \
  -I lib -I $(srcdir)/lib \
  -DBUILDING_LIBADFTOOL_PYTHON \
  $(COMMON_COMPILE_FLAGS)
adftool_la_CFLAGS = $(CFLAG_VISIBILITY)
adftool_la_LIBADD = libadftool.la lib/libadftool.la $(LTLIBOBJS)
adftool_la_LDFLAGS += \
  -no-undefined -avoid-version -module \
  $(LTLIBINTL)
endif

if RELOCATABLE_VIA_LD
libadftool_la_LDFLAGS += `$(RELOCATABLE_LDFLAGS) $(libdir)`
adftool_la_LDFLAGS += `$(RELOCATABLE_LDFLAGS) $(bindir)`
endif

AM_TESTS_ENVIRONMENT = \
  mkdir -p $(abs_top_builddir)/.tmp ; \
  export TMPDIR=$(abs_top_builddir)/.tmp ;

BUILT_SOURCES = src/filter_test_data.c
src_filter_example_SOURCES = src/filter_example.c src/filter_test_data.c

$(srcdir)/src/filter_test_data.c: src/filter_test_data.R src/filter_test_data.csv
	$(AM_V_GEN) (cd $(srcdir) && $(RSCRIPT) src/filter_test_data.R > src/filter_test_data.c-t)
	@mv $@-t $@

$(top_srcdir)/.version:
	$(AM_V_GEN) echo '$(VERSION)' > $@-t
	@mv $@-t $@

$(top_srcdir)/conanfile.py: conanfile-in.py
	$(AM_V_GEN) $(SED) 's/\@VERSION\@/$(VERSION)/g' $< > $@-t
	@mv $@-t $@

$(top_srcdir)/src/js/package.json: src/js/package.json.in
	$(AM_V_GEN) $(SED) "s/\\@VERSION\\@/$$(echo $(VERSION) | cut -d '.' -f 1-3)/g" $< > $@-t
	@mv $@-t $@

dist-hook:
	echo '$(VERSION)' > $(distdir)/.tarball-version

if NODE_MODULE
EXPORTED_FUNCTIONS = _adftool_add_channel_type,_adftool_array_double_address,_adftool_array_double_alloc,_adftool_array_double_free,_adftool_array_double_get,_adftool_array_double_set,_adftool_array_long_address,_adftool_array_long_alloc,_adftool_array_long_free,_adftool_array_long_get,_adftool_array_long_set,_adftool_array_size_t_address,_adftool_array_size_t_alloc,_adftool_array_size_t_free,_adftool_array_size_t_get,_adftool_array_size_t_set,_adftool_array_uint64_t_address,_adftool_array_uint64_t_alloc,_adftool_array_uint64_t_free,_adftool_array_uint64_t_get_js_high,_adftool_array_uint64_t_get_js_low,_adftool_array_uint64_t_set_js,_adftool_delete,_adftool_dictionary_get,_adftool_dictionary_insert,_adftool_dictionary_lookup,_adftool_eeg_get_data,_adftool_eeg_get_time,_adftool_eeg_set_data,_adftool_eeg_set_time,_adftool_file_close,_adftool_file_get_data,_adftool_file_open,_adftool_file_open_data,_adftool_find_channel_identifier,_adftool_find_channels_by_type,_adftool_fir_auto_bandwidth,_adftool_fir_auto_order,_adftool_fir_alloc,_adftool_fir_apply,_adftool_fir_design_bandpass,_adftool_fir_free,_adftool_fir_order,_adftool_get_channel_column,_adftool_get_channel_decoder,_adftool_get_channel_types,_adftool_insert,_adftool_lookup,_adftool_lookup_objects,_adftool_lookup_subjects,_adftool_quads_delete,_adftool_quads_get,_adftool_quads_insert,_adftool_set_channel_decoder,_adftool_statement_alloc,_adftool_statement_compare,_adftool_statement_copy,_adftool_statement_free,_adftool_statement_get,_adftool_statement_set,_adftool_term_alloc,_adftool_term_as_date,_adftool_term_as_double,_adftool_term_as_integer,_adftool_term_as_mpf,_adftool_term_as_mpz,_adftool_term_compare,_adftool_term_copy,_adftool_term_decode,_adftool_term_encode,_adftool_term_free,_adftool_term_is_blank,_adftool_term_is_langstring,_adftool_term_is_literal,_adftool_term_is_named,_adftool_term_is_typed_literal,_adftool_term_meta,_adftool_term_parse_n3,_adftool_term_set_blank,_adftool_term_set_date,_adftool_term_set_double,_adftool_term_set_integer,_adftool_term_set_literal,_adftool_term_set_mpf,_adftool_term_set_mpz,_adftool_term_set_named,_adftool_term_to_n3,_adftool_term_value,_adftool_timespec_alloc,_adftool_timespec_free,_adftool_timespec_set_js,_adftool_timespec_get_js

src/js/index.mjs: libadftool.la
	mkdir -p src/js/
	./libtool --tag=CC --mode=link $(CC) -O3 $< -sEXPORT_ALL=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,stringToUTF8,lengthBytesUTF8 -sEXPORTED_FUNCTIONS=_malloc,_free,$(EXPORTED_FUNCTIONS) $(CFLAGS) $(LDFLAGS) $(LIBS) -o src/js/index.mjs -sSINGLE_FILE

nodejsdir = $(datadir)/nodejs
pkgnodejsdir = $(nodejsdir)/$(PACKAGE)
pkgnodejs_DATA = src/js/index.mjs src/js/package.json
dist_pkgnodejs_DATA = \
  src/js/adftool_binding.mjs \
  src/js/adftool_double_array.mjs \
  src/js/adftool_file.mjs \
  src/js/adftool_fir.mjs \
  src/js/adftool_load_binding.mjs \
  src/js/adftool_long_array.mjs \
  src/js/adftool.mjs \
  src/js/adftool_numeric_array.mjs \
  src/js/adftool_pointer_array.mjs \
  src/js/adftool_size_t_array.mjs \
  src/js/adftool_statement.mjs \
  src/js/adftool_string.mjs \
  src/js/adftool_term.mjs \
  src/js/adftool_timespec.mjs \
  src/js/adftool_uint64_t_array.mjs \
  src/js/test.mjs
endif
