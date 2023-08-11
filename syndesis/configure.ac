#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.64)
AC_INIT([syndesis],[0.3.1],[kkapelon@freemail.gr])


AM_INIT_AUTOMAKE()

# Checks for programs.
AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES(DEPS, glib-2.0 gtk+-2.0 cairo libcurl gthread-2.0)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([math.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset])

AC_SUBST([pkgdatadir])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

echo
echo "Finished configuring $PACKAGE $VERSION"
echo
echo "Now type 'make' (gmake on some systems) to compile $PACKAGE"
echo

