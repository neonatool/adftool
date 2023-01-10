#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([adftool], m4_esyscmd([sh build-aux/git-version-gen .tarball-version]), [vivien.kraus@univ-reims.fr])
AC_CONFIG_SRCDIR([include/adftool.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_CONDITIONAL([SHLIBS_IN_BINDIR],
               [case "$host_os" in mingw* | cygwin*) true;; *) false;; esac])

AC_ARG_ENABLE([python],
  [  --enable-python=yes/no  Build or skip python bindings],
  [case "${enableval}" in
    no)   python_module=false ;;
    yes)  python_module=true ;;
    *)    AC_MSG_ERROR([Bad value ${enableval} for --enable-python]) ;;
   esac], [python_module=true])

AM_CONDITIONAL([PYTHON_MODULE], [test x$python_module = xtrue])

AC_ARG_ENABLE([node],
  [  --enable-node=yes Install a nodejs module (requires an emscripten configuration)],
  [case "${enableval}" in
    no)  node_module=false ;;
    yes) node_module=true ;;
    *)    AC_MSG_ERROR([Bad value ${enableval} for --enable-node]) ;;
   esac], [node_module=false])

AM_CONDITIONAL([NODE_MODULE], [test x$node_module = xtrue])

AC_ARG_ENABLE([js-initial-memory],
  [  --enable-js-initial-memory=BYTES Request at least BYTES memory of heap space when running the wasm output],
  [case "${enableval}" in
    yes)
	# https://english.stackexchange.com/questions/504260/origin-of-yes-joke-to-a-question
	# This is 2 GiB
	CFLAGS="$CFLAGS -sINITIAL_MEMORY=2147483648" ;;
    no) ;;
    default) ;;
    *)
	CFLAGS="$CFLAGS -sINITIAL_MEMORY=${enableval}" ;;
  esac])

# Checks for programs.
AC_PROG_CC
gl_EARLY
glprog_EARLY
AM_INIT_AUTOMAKE([gnu -Wall -Werror subdir-objects])
AM_PROG_AR
LT_INIT([win32-dll])
AM_PROG_LEX([noyywrap])
if test "x$LEX" != xflex; then
  AM_MISSING_PROG([LEX], [flex])
  AC_SUBST([LEX_OUTPUT_ROOT], [lex.yy])
  AC_SUBST([LEXLIB], [''])
fi
gl_INIT
glprog_INIT
AM_MISSING_PROG([RSCRIPT], [Rscript])
AM_PATH_PYTHON(,, [
  if test x$python_module = xtrue ; then
    AC_MSG_ERROR([Python module was required, but python has not been found. Pass --disable-python to not build the python extension.])
  fi
])
AC_PROG_SED

# Checks for libraries.
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19])
PKG_CHECK_MODULES([HDF5], [hdf5], [
  CFLAGS="$CFLAGS $HDF5_CFLAGS"
  LIBS="$LIBS $HDF5_LIBS"
], [AC_MSG_WARN([No hdf5 pkg-config module found.])])
AC_SEARCH_LIBS([H5open], [hdf5],,, [-lz])
AC_SEARCH_LIBS([__gmpz_init], [gmp])
AC_SEARCH_LIBS([cos], [m])
PKG_CHECK_MODULES([PYTHON3], [python3], [
  CFLAGS="$CFLAGS $PYTHON3_CFLAGS"
  LIBS="$LIBS $PYTHON3_LIBS"
], [AC_MSG_WARN([No python pkg-config module found.])])
PKG_CHECK_MODULES([BPLUS], [bplus bplus-hdf5], [
  CFLAGS="$CFLAGS $BPLUS_CFLAGS"
  CPPFLAGS="$CPPFLAGS $BPLUS_CPPFLAGS"
  LIBS="$LIBS $BPLUS_LIBS"
], [AC_MSG_WARN([Pkg-config could not find bplus or bplus-hdf5.])])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h emscripten.h])
AC_CHECK_HEADER([hdf5.h],, [AC_MSG_ERROR([I need the HDF5 header.])])
AC_CHECK_HEADER([gmp.h],, [AC_MSG_ERROR([I need the gmp.h header.])])
AC_CHECK_HEADER([bplus.h],, [AC_MSG_ERROR([I need the bplus.h header from libbplus.])])
AC_CHECK_HEADER([bplus_hdf5.h],, [AC_MSG_ERROR([I need the bplus.h header from libbplus.])])
AC_CHECK_HEADER([Python.h],, [
  if test x$python_module = xtrue ; then
    AC_MSG_ERROR([Python module was required, but Python.h cannot be used.])
  fi])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
gl_VISIBILITY

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNC([H5open],, [AC_MSG_ERROR([I need the HDF5 library functions.])])
AC_CHECK_FUNC([__gmpz_init],, [AC_MSG_ERROR([I need the GMP library functions.])])
AC_CHECK_FUNC([bplus_find],, [AC_MSG_ERROR([I need the libbplus library functions.])])
AC_CHECK_FUNC([bplus_hdf5_table_alloc],, [AC_MSG_ERROR([I need the HDF5 extension of libbplus.])])
AC_CHECK_FUNCS([localeconv])

AC_CONFIG_FILES([
  Makefile
  lib/Makefile libprog/Makefile
  po/Makefile.in
  tests/Makefile])
AC_OUTPUT
