#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([adftool], m4_esyscmd([sh build-aux/git-version-gen .tarball-version]), [vivien.kraus@univ-reims.fr])
AC_CONFIG_SRCDIR([src/libadftool/lookup.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_ARG_WITH([hdf5],
  AS_HELP_STRING([--without-hdf5], [Do not build HDF5 bindings. This is a very bad idea, but it’s the only way to get adftool work with emscripten (go to hell, emscripten, and please bring npm to the party)]))

# Checks for programs.
AC_PROG_CC
gl_EARLY
AM_INIT_AUTOMAKE([gnu -Wall -Werror subdir-objects])
AM_PROG_AR
LT_INIT([win32-dll])
gl_INIT

# Checks for libraries.
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.21])

AS_IF([test "x$with_hdf5" != "xno"], [
  AC_SEARCH_LIBS([H5open], [hdf5],,, [-lz])
])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h])
AS_IF([test "x$with_hdf5" != "xno"], [
  AC_CHECK_HEADER([hdf5.h],, [AC_MSG_ERROR([I need the HDF5 header.])])
])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AS_IF([test "x$with_hdf5" != "xno"], [
  AC_CHECK_FUNC([H5open],, [AC_MSG_ERROR([I need the HDF5 library functions.])])
])

AM_CONDITIONAL([WITH_HDF5], [test "x$with_hdf5" != "xno"])
AC_CONFIG_FILES([Makefile lib/Makefile po/Makefile.in])
AC_OUTPUT
