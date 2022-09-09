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
AC_SEARCH_LIBS([H5open], [hdf5],,, [-lz])
AC_SEARCH_LIBS([__gmpz_init], [gmp])
AC_SEARCH_LIBS([cos], [m])
PKG_CHECK_MODULES([PYTHON3], [python3], [
  CFLAGS="$CFLAGS $PYTHON3_CFLAGS"
  LIBS="$LIBS $PYTHON3_LIBS"
], [AC_MSG_WARN([No python pkg-config module found.])])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h emscripten.h])
AC_CHECK_HEADER([hdf5.h],, [AC_MSG_ERROR([I need the HDF5 header.])])
AC_CHECK_HEADER([gmp.h],, [AC_MSG_ERROR([I need the gmp.h header.])])
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
AC_CHECK_FUNCS([localeconv])

AC_CONFIG_FILES([
  Makefile
  lib/Makefile libprog/Makefile
  po/Makefile.in
  tests/Makefile])
AC_OUTPUT
