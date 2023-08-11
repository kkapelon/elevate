#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([vault], 0.2.0, [kkapelon@freemail.gr])


AM_INIT_AUTOMAKE()

# Checks for programs.
AC_ISC_POSIX
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES(DEPS, glib-2.0 sqlite3 >= 3.5)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strcasecmp strchr strdup strncasecmp strpbrk])

AC_SUBST([pkgdatadir])
AC_OUTPUT([Makefile src/Makefile])

echo
echo "Finished configuring $PACKAGE $VERSION"
echo
echo "Now type 'make' (gmake on some systems) to compile $PACKAGE"
echo

