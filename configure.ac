#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([adftool], m4_esyscmd([sh build-aux/git-version-gen .tarball-version]), [vivien.kraus@univ-reims.fr])
AC_CONFIG_SRCDIR([src/libadftool/lookup.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
gl_EARLY
AC_PROG_CXX
AM_INIT_AUTOMAKE([gnu -Wall -Werror subdir-objects])
AM_PROG_AR
LT_INIT([win32-dll])
gl_INIT

# Checks for libraries.
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19])
AC_SEARCH_LIBS([H5open], [hdf5],,, [-lz])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h emscripten.h emscripten/bind.h])
AC_CHECK_HEADER([hdf5.h],, [AC_MSG_ERROR([I need the HDF5 header.])])

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

AC_CONFIG_FILES([Makefile lib/Makefile po/Makefile.in])
AC_OUTPUT
