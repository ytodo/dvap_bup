#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(dv_ap, 01.00, 7m3tjz@jarl.com)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lusb':
AC_CHECK_LIB([usb], [main])
AC_CHECK_LIB([crypto], [main])

# Checks for header files.
AC_CHECK_HEADERS([netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday memset select socket strerror tzset uname])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
