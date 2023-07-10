#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([adftool], m4_esyscmd([sh build-aux/git-version-gen .tarball-version]), [vivien.kraus@univ-reims.fr])

AC_SUBST(LTV_CURRENT, [3])
AC_SUBST(LTV_REVISION, [0])
AC_SUBST(LTV_AGE, [3])

AC_SUBST(DLL_VERSION, $(expr ${LTV_CURRENT} - ${LTV_AGE}))

AC_CONFIG_SRCDIR([include/adftool.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_ARG_ENABLE([python],
  [  --enable-python=yes/no  Build or skip python bindings],
  [case "${enableval}" in
    no)   python_module=false ;;
    yes)  python_module=true ;;
    *)    AC_MSG_ERROR([Bad value ${enableval} for --enable-python]) ;;
   esac], [python_module=true])

AM_CONDITIONAL([PYTHON_MODULE], [test x$python_module = xtrue])

AC_ARG_ENABLE([r],
  [  --enable-r=yes/no  Build or skip R bindings],
  [case "${enableval}" in
    no)   r_module=false ;;
    yes)  r_module=true ;;
    *)    AC_MSG_ERROR([Bad value ${enableval} for --enable-r]) ;;
   esac], [r_module=true])

AM_CONDITIONAL([R_MODULE], [test x$r_module = xtrue])

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

DEFAULT_ONTOLOGY_PREFIX="https://neonatool.github.io/slam/ontology/lytonepal.en.html#"

AC_ARG_VAR([LYTONEPAL_ONTOLOGY_PREFIX], [The lytonepal ontology prefix, with the final \# character if needed.])

if test "x$LYTONEPAL_ONTOLOGY_PREFIX" = "x"
then
  LYTONEPAL_ONTOLOGY_PREFIX="$DEFAULT_ONTOLOGY_PREFIX"
fi

AC_DEFINE_UNQUOTED([LYTONEPAL_ONTOLOGY_PREFIX], ["$LYTONEPAL_ONTOLOGY_PREFIX"], [The Lytonepal ontology prefix])

# Checks for programs.
AC_PROG_CC
gl_EARLY
glprog_EARLY
AC_PROG_CXX
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
if test "x$r_module" = "xtrue"
then
  AX_CXX_COMPILE_STDCXX([17])
fi
AC_PATH_PROG([CYGPATH_WP], [cygpath], [no])
if test "x$CYGPATH_WP" = xno
then
  CYGPATH_WP=echo
else
  CYGPATH_WP="$CYGPATH_WP -w -p"
fi

# Checks for libraries.
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19])
PKG_CHECK_MODULES([HDF5], [hdf5], [
  CPPFLAGS="$CPPFLAGS $HDF5_CFLAGS"
  CFLAGS="$CFLAGS $HDF5_CFLAGS"
  LIBS="$LIBS $HDF5_LIBS"
], [AC_MSG_WARN([No hdf5 pkg-config module found.])])
AC_SEARCH_LIBS([H5open], [hdf5],,, [-lz])
AC_SEARCH_LIBS([__gmpz_init], [gmp])
AC_SEARCH_LIBS([cos], [m])
PKG_CHECK_MODULES([PYTHON3], [python3], [
  CPPFLAGS="$CPPFLAGS $PYTHON3_CPPFLAGS"
  CFLAGS="$CFLAGS $PYTHON3_CFLAGS"
  LIBS="$LIBS $PYTHON3_LIBS"
], [AC_MSG_WARN([No python pkg-config module found.])])
PKG_CHECK_MODULES([LIBCHECK], [check], [
  CPPFLAGS="$CPPFLAGS $LIBCHECK_CFLAGS"
  CFLAGS="$CFLAGS $LIBCHECK_CFLAGS"
  LIBS="$LIBS $LIBCHECK_LIBS"
], [AC_MSG_WARN([pkg-config does not know the "check" module])])
AC_ARG_VAR([R_CXXFLAGS], [R compile-time flags for c++. By default, ask R CMD config CPP/CXXFLAGS, and add -I to elements of R.home("include").])
AC_ARG_VAR([R_LIBS], [R link-time flags. By default, ask R CMD config LDFLAGS, and add -L to elements of R.home("lib").])
AC_ARG_VAR([RCPP_CXXFLAGS], [Rcpp compile-time flags. By default, ask Rcpp:::CxxFlags().])
AC_ARG_VAR([RCPP_LIBS], [Rcpp link-time flags. By default, ask Rcpp:::LdFlags().])
if test "x$r_module" = "xtrue"
then
  AC_PATH_PROG([RSCRIPT], [Rscript], [Rscript])
  AC_PATH_PROG([RREPL], [R], [R])
  if test "x$R_CXXFLAGS" = "x" && test "x$R_LIBS" = "x"
  then
    AC_MSG_CHECKING([for R compile flags])
    R_CXXFLAGS="$($RREPL CMD config CPPFLAGS) $($RREPL CMD config CXXFLAGS) -I$($RSCRIPT -e 'cat (R.home ("include"))')"
    if test "x$R_CXXFLAGS" = "x"
    then
      AC_MSG_WARN([I cannot query the R compilation flags.])
      AC_MSG_RESULT([none provided])
    else
      AC_MSG_RESULT([$R_CXXFLAGS])
    fi
    AC_MSG_CHECKING([for R libraries to link])
    R_LIBS="$($RREPL CMD config LDFLAGS) -L$($RSCRIPT -e 'cat (R.home ("lib"))')"
    if test "x$R_LIBS" = "x"
    then
      AC_MSG_WARN([I cannot query the R link flags.])
      AC_MSG_RESULT([none provided])
    else
      AC_MSG_RESULT([$R_LIBS])
    fi
  fi
  if test "x$RCPP_CXXFLAGS" = "x" && test "x$RCPP_LIBS" = "x"
  then
    AC_MSG_CHECKING([for Rcpp compile flags])
    RCPP_CXXFLAGS="$($RSCRIPT -e 'Rcpp:::CxxFlags()' | $SED -s 's/\"//g')"
    if test "x$RCPP_CXXFLAGS" = "x"
    then
      AC_MSG_WARN([I cannot query the Rcpp compilation flags.])
      AC_MSG_RESULT([none provided])
    else
      AC_MSG_RESULT([$RCPP_CXXFLAGS])
    fi
    AC_MSG_CHECKING([for Rcpp libraries to link])
    RCPP_LIBS=$($RSCRIPT -e 'Rcpp:::LdFlags()')
    if test "x$RCPP_LIBS" = "x"
    then
      AC_MSG_RESULT([none provided])
    else
      AC_MSG_RESULT([$RCPP_LIBS])
    fi
  fi
fi


# Checks for header files.
AC_CHECK_HEADERS([stdint.h emscripten.h check.h])
AC_CHECK_HEADER([hdf5.h],, [AC_MSG_ERROR([I need the HDF5 header.])])
AC_CHECK_HEADER([gmp.h],, [AC_MSG_ERROR([I need the gmp.h header.])])
AC_CHECK_HEADER([Python.h],, [
  if test x$python_module = xtrue ; then
    AC_MSG_ERROR([Python module was required, but Python.h cannot be used.])
  fi])
if test "x$r_module" = "xtrue"
then
  AC_LANG_PUSH([C++])
  SAVE_CPPFLAGS="$CPPFLAGS"
  SAVE_CXXFLAGS="$CXXFLAGS"
  CPPFLAGS="$CPPFLAGS $R_CXXFLAGS $RCPP_CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $R_CXXFLAGS $RCPP_CXXFLAGS"
  AC_CHECK_HEADER([Rcpp.h],, [AC_MSG_ERROR([Disable the R module if rcpp is not present.])])
  CPPFLAGS="$SAVE_CPPFLAGS"
  CXXFLAGS="$SAVE_CXXFLAGS"
  AC_LANG_POP()
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
gl_VISIBILITY
SAVE_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$SAVE_CPPFLAGS -I$srcdir/include"
AC_CHECK_ALIGNOF([struct bplus_node], [[#include <bplus.h>]])
AC_CHECK_ALIGNOF([struct bplus_key], [[#include <bplus.h>]])
CPPFLAGS="$SAVE_CPPFLAGS"

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNC([H5open],, [AC_MSG_ERROR([I need the HDF5 library functions.])])
AC_CHECK_FUNC([__gmpz_init],, [AC_MSG_ERROR([I need the GMP library functions.])])
AC_SEARCH_LIBS([srunner_create], [check])

if test "x$r_module" = "xtrue"
then
  AC_MSG_CHECKING([whether Rcpp can be linked])
  AC_LANG_PUSH([C++])
  SAVE_CPPFLAGS="$CPPFLAGS"
  SAVE_CXXFLAGS="$CXXFLAGS"
  SAVE_LIBS="$LIBS"
  CPPFLAGS="$CPPFLAGS $R_CXXFLAGS $RCPP_CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $R_CXXFLAGS $RCPP_CXXFLAGS"
  LIBS="$LIBS $R_LIBS $RCPP_LIBS"
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([[#include <Rcpp.h>]],
                     [[Rcpp::NumericMatrix m;]])],
    [AC_MSG_RESULT([yes])],
    [ LIBS="$LIBS -lR"
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM([[#include <Rcpp.h>]],
                         [[Rcpp::NumericMatrix m;]])],
        [ AC_MSG_RESULT([yes, with -lR])
	  R_LIBS="$R_LIBS -lR" ],
	[ AC_MSG_RESULT([no])
          AC_MSG_ERROR([Disable the R module if rcpp is not present.])])])
  CPPFLAGS="$SAVE_CPPFLAGS"
  CXXFLAGS="$SAVE_CXXFLAGS"
  LIBS="$SAVE_LIBS"
  AC_LANG_POP()
fi

host_is_windows=no
case "$host_os" in
  mingw* | cygwin*)
    host_is_windows=yes;;
esac

AM_CONDITIONAL([SHLIBS_IN_BINDIR], [test "x$i_am_windows" = "xyes"])
AM_CONDITIONAL([WINDOWS], [test "x$host_is_windows" = "xyes"])

AC_CONFIG_FILES([
  Makefile
  lib/Makefile libprog/Makefile
  po/Makefile.in
  tests/Makefile])
AC_OUTPUT
