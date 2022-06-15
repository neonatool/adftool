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
AM_INIT_AUTOMAKE([gnu -Wall -Werror subdir-objects])
AM_PROG_AR
LT_INIT([win32-dll])
gl_INIT

# Checks for libraries.
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.21])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_CONFIG_FILES([Makefile lib/Makefile po/Makefile.in])
AC_OUTPUT
