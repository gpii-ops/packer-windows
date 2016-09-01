# autoconf input for the Unix side of doit.

AC_INIT([doit], [20160308.016c16c], [anakin@pobox.com])
AC_CONFIG_SRCDIR([doit.c])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE(foreign)

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_INSTALL

AC_SEARCH_LIBS(connect, socket nsl)
AC_SEARCH_LIBS(inet_ntoa, socket nsl)
AC_SEARCH_LIBS(inet_addr, socket nsl)
AC_SEARCH_LIBS(gethostbyname, socket nsl resolv)

AC_CHECK_FUNCS([realpath])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
