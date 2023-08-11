#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.64)
AC_INIT([elevate],0.3.0,[kkapelon@freemail.gr])

AM_INIT_AUTOMAKE()

# Checks for programs.
AC_ISC_POSIX
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES(DEPS, gtk+-2.0 cairo glib-2.0)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST


AC_OUTPUT([Makefile src/Makefile modules/Makefile])

echo
echo "Finished configuring $PACKAGE $VERSION"
echo
echo "Now type 'make' (gmake on some systems) to compile $PACKAGE"
echo

