#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.64)
AC_INIT([central_hub],0.2.0,[kkapelon@freemail.gr])

AM_INIT_AUTOMAKE()

# Checks for programs.
AC_ISC_POSIX
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES(DEPS, glib-2.0 gtk+-2.0 cairo)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/vfs.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gethostbyname inet_ntoa memset socket])

AC_OUTPUT([Makefile src/Makefile])

echo
echo "Finished configuring $PACKAGE $VERSION"
echo
echo "Now type 'make' (gmake on some systems) to compile $PACKAGE"
echo

