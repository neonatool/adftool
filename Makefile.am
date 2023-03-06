SUBDIRS = lib libprog . po tests
ACLOCAL_AMFLAGS = -I m4 -I m4prog
EXTRA_DIST = m4/gnulib-cache.m4 m4prog/gnulib-cache.m4 \
  src/filter_test_data.c src/filter_test_data.R src/filter_test_data.csv \
  po/Rules-install-local src/test.py \
  $(top_srcdir)/.version \
  $(top_srcdir)/src/js/package.json.in \
  src/test-R.R

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
  filter-log \
  test.adf

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

AM_DISTCHECK_CONFIGURE_FLAGS = SHELL=$(SHELL)

RELOCATABLE_LIBRARY_PATH = $(libdir)

bin_PROGRAMS = src/adftool

lib_LTLIBRARIES = libadftool.la
dist_include_HEADERS = include/adftool.h
check_PROGRAMS = \
  src/file_open_example \
  src/dictionary_unit_test \
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
  src/simple_filters \
  src/check_bplus \
  src/typed_literal_lookup \
  src/encoding_unit_test

TESTS = $(check_PROGRAMS)

TEST_EXTENSIONS = .py .R

LOG_COMPILER = $(LOG_VALGRIND)

PY_LOG_COMPILER = $(PYTHON)
R_LOG_COMPILER = $(RSCRIPT)

src_adftool_LDADD = \
  libprog/libgnu.la \
  $(LTLIBOBJS) \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  libadftool.la

libadftool_la_SOURCES = \
  ./include/bplus.h \
  src/libbplus/bplus_hdf5.h \
  src/libbplus/bplus_analyzer.h \
  src/libbplus/bplus_divider.h \
  src/libbplus/bplus_explorer.h \
  src/libbplus/bplus_fetch.h \
  src/libbplus/bplus_fetcher.h \
  src/libbplus/bplus_find.h \
  src/libbplus/bplus_finder.h \
  src/libbplus/bplus_growth.h \
  src/libbplus/bplus_insert.h \
  src/libbplus/bplus_insertion.h \
  src/libbplus/bplus_key.h \
  src/libbplus/bplus_node.h \
  src/libbplus/bplus_parent_fetcher.h \
  src/libbplus/bplus_prime.h \
  src/libbplus/bplus_range.h \
  src/libbplus/bplus_reparentor.h \
  src/libbplus/bplus_tree.h \
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
  src/libadftool/literal_filter_iterator.h \
  src/libadftool/quads.h \
  src/libadftool/quads_index.h \
  src/libadftool/statement.c \
  src/libadftool/statement.h \
  src/libadftool/term.c \
  src/libadftool/term.h \
  src/libadftool/version.c
libadftool_la_CPPFLAGS = \
  -I lib -I $(srcdir)/lib \
  -I $(abs_top_builddir)/src/libadftool -I $(abs_top_srcdir)/src/libadftool \
  -DBUILDING_LIBADFTOOL \
  $(COMMON_COMPILE_FLAGS)
libadftool_la_CFLAGS = \
    $(CFLAG_VISIBILITY)
libadftool_la_LFLAGS = --prefix=_adftool_lexer_
libadftool_la_LIBADD = lib/libadftool.la $(LTLIBOBJS)
libadftool_la_LDFLAGS = \
  -no-undefined \
  -version-info ${LTV_CURRENT}:${LTV_REVISION}:${LTV_AGE} \
  $(LTLIBINTL) \
  $(LIB_CLOCK_GETTIME) \
  $(LIB_GETRANDOM) \
  $(MODF_LIBM) \
  $(TRUNC_LIBM) \
  $(LIB_SETLOCALE) \
  $(LIB_SETLOCALE_NULL) \
  $(COMMON_LINK_FLAGS) \
  $(LIBTHREAD) \
  $(INTL_MACOSX_LIBS)

AM_TESTS_ENVIRONMENT = \
  mkdir -p $(abs_top_builddir)/.tmp ; \
  export TMPDIR=$"$$($(CYGPATH_W) $(abs_top_builddir)/.tmp)$" ; \
  export PYTHONPATH=$"$$($(CYGPATH_WP) "$(abs_top_builddir)/.libs$(PATH_SEPARATOR)$(abs_top_builddir)/src/.libs$(PATH_SEPARATOR)$$PYTHONPATH")$" ;

.PHONY: print-tests-environment run-in-tests-environment

print-tests-environment: $(lib_LTLIBRARIES)
	@echo "# Set by adftool: $(AM_TESTS_ENVIRONMENT)"
	@echo "# Set by the TESTS_ENVIRONMENT make variable: $(TESTS_ENVIRONMENT)"
	@echo "$(AM_TESTS_ENVIRONMENT) $(TESTS_ENVIRONMENT)"

run-in-tests-environment: $(lib_LTLIBRARIES)
	@$(AM_TESTS_ENVIRONMENT) $(TESTS_ENVIRONMENT) $(COMMAND)

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

if WINDOWS
adftool_la_LDFLAGS += -shrext .pyd
endif

TESTS += src/test.py
endif

if R_MODULE
lib_LTLIBRARIES += src/libadftool-r.la
src_libadftool_r_la_SOURCES = \
  src/adftool_r.cc
src_libadftool_r_la_CPPFLAGS = \
  -I lib -I $(srcdir)/lib \
  -DBUILDING_LIBADFTOOL_R \
  $(COMMON_COMPILE_FLAGS)
src_libadftool_r_la_CXXFLAGS = $(CFLAG_VISIBILITY) $(R_CXXFLAGS) $(RCPP_CXXFLAGS)
src_libadftool_r_la_LIBADD = libadftool.la lib/libadftool.la $(LTLIBOBJS) $(R_LIBS) $(RCPP_LIBS)
src_libadftool_r_la_LDFLAGS = \
  -no-undefined -avoid-version -module \
  $(LTLIBINTL)

TESTS += src/test-R.R

AM_TESTS_ENVIRONMENT += \
  export LIBADFTOOL_R_LIBRARY=$"$$($(CYGPATH_W) $$(. src/libadftool-r.la ; echo "$(abs_top_builddir)/src/.libs/$$dlname"))$" ;
endif

if WINDOWS
## Windows forbids loading of DLLs if they are not in PATH (this is
## not about discoverability). WARNING: anything that comes after that
## in AM_TESTS_ENVIRONMENT, or anything in TESTS_ENVIRONMENT, will not
## have a PATH usable by msys2/cygwin.
AM_TESTS_ENVIRONMENT += \
  export PATH=$"$$($(CYGPATH_WP) "$(abs_top_builddir)/.libs$(PATH_SEPARATOR)$(abs_top_builddir)/src/.libs$(PATH_SEPARATOR)$$PATH")$";
endif

DISTCLEANFILES =

if RELOCATABLE_VIA_LD
libadftool_la_LDFLAGS += `$(RELOCATABLE_LDFLAGS) $(libdir)`
adftool_la_LDFLAGS += `$(RELOCATABLE_LDFLAGS) $(bindir)`
endif

if HAVE_LD_OUTPUT_DEF
libadftool_la_LDFLAGS += \
  -Wl,--output-def,libadftool-$(DLL_VERSION).def

libadftool-$(DLL_VERSION).def: libadftool.la

defexecdir = $(libdir)
defexec_DATA = libadftool-$(DLL_VERSION).def

DISTCLEANFILES += $(defexec_DATA)
endif

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
EXPORTED_FUNCTIONS = _adftool_add_channel_type,_adftool_array_double_address,_adftool_array_double_alloc,_adftool_array_double_free,_adftool_array_double_get,_adftool_array_double_set,_adftool_array_long_address,_adftool_array_long_alloc,_adftool_array_long_free,_adftool_array_long_get,_adftool_array_long_set,_adftool_array_size_t_address,_adftool_array_size_t_alloc,_adftool_array_size_t_free,_adftool_array_size_t_get,_adftool_array_size_t_set,_adftool_array_uint64_t_address,_adftool_array_uint64_t_alloc,_adftool_array_uint64_t_free,_adftool_array_uint64_t_get_js_high,_adftool_array_uint64_t_get_js_low,_adftool_array_uint64_t_set_js,_adftool_delete,_adftool_eeg_get_data,_adftool_eeg_get_time,_adftool_eeg_set_data,_adftool_eeg_set_time,_adftool_file_close,_adftool_file_get_data,_adftool_file_open,_adftool_file_open_data,_adftool_find_channel_identifier,_adftool_find_channels_by_type,_adftool_fir_auto_bandwidth,_adftool_fir_auto_order,_adftool_fir_alloc,_adftool_fir_apply,_adftool_fir_design_bandpass,_adftool_fir_free,_adftool_fir_order,_adftool_get_channel_column,_adftool_get_channel_decoder,_adftool_get_channel_types,_adftool_insert,_adftool_lookup,_adftool_lookup_objects,_adftool_lookup_subjects,_adftool_quads_delete,_adftool_quads_get,_adftool_quads_insert,_adftool_set_channel_decoder,_adftool_statement_alloc,_adftool_statement_compare,_adftool_statement_copy,_adftool_statement_free,_adftool_statement_get,_adftool_statement_set,_adftool_term_alloc,_adftool_term_as_date,_adftool_term_as_double,_adftool_term_as_integer,_adftool_term_as_mpf,_adftool_term_as_mpz,_adftool_term_compare,_adftool_term_copy,_adftool_term_decode,_adftool_term_encode,_adftool_term_free,_adftool_term_is_blank,_adftool_term_is_langstring,_adftool_term_is_literal,_adftool_term_is_named,_adftool_term_is_typed_literal,_adftool_term_meta,_adftool_term_parse_n3,_adftool_term_set_blank,_adftool_term_set_date,_adftool_term_set_double,_adftool_term_set_integer,_adftool_term_set_literal,_adftool_term_set_mpf,_adftool_term_set_mpz,_adftool_term_set_named,_adftool_term_to_n3,_adftool_term_value,_adftool_timespec_alloc,_adftool_timespec_free,_adftool_timespec_set_js,_adftool_timespec_get_js

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
