#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

AC_INIT([pdf2laser], [0.3.2], [https://github.com/zellio/pdf2laser/issues], [pdf2laser])
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/pdf2laser.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_SEARCH_LIBS([sqrt], [m])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h limits.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([alarm gethostname inet_ntoa socket sqrt strchr strdup strncasecmp strrchr])

AC_CONFIG_FILES([src/Makefile
doc/Makefile
extra/Makefile
extra/completion/Makefile
extra/completion/bash/Makefile
extra/completion/zsh/Makefile
Makefile])

AC_OUTPUT
