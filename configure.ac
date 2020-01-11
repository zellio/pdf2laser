#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

AC_INIT([pdf2laser], [0.5.0], [https://github.com/zellio/pdf2laser/issues], [pdf2laser])
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/pdf2laser.c])
AC_CONFIG_HEADERS([config.h])

# Define configurable defaults.
AC_ARG_VAR([DEFAULT_HOST], [Default host address when -p/--printer is not specified. If not overridden, defaults to the NYC Resistor laser host (192.168.1.4).])
AC_DEFINE_UNQUOTED([DEFAULT_HOST], ["${DEFAULT_HOST=192.168.1.4}"], [Default host address when -p/--printer is not specified.])

# Checks for programs.
AC_PROG_CC([clang gcc])
AC_PROG_INSTALL

# Checks for libraries.
AC_SEARCH_LIBS([sqrt], [m])
AC_SEARCH_LIBS([gsapi_new_instance], [gs])

# Checks for header files.
AC_CHECK_HEADERS([ \
arpa/inet.h \
ctype.h \
errno.h \
getopt.h \
ghostscript/gserrors.h \
ghostscript/iapi.h \
libgen.h \
math.h \
netdb.h \
netinet/in.h \
stdbool.h \
stdint.h \
stdio.h \
stdlib.h \
string.h \
strings.h \
sys/sendfile.h \
sys/socket.h \
sys/stat.h \
sys/types.h \
unistd.h \
])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT8_T
AC_TYPE_UINT8_T
AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_CHECK_FUNCS([ \
addrinfo \
alarm \
atoi \
basename \
calloc \
close \
connect \
errno \
exit \
fclose \
fileno \
fopen \
fprintf \
free \
freeaddrinfo \
fstat \
getaddrinfo \
gs_error_type \
gsapi_delete_instance \
gsapi_exit \
gsapi_init_with_args \
gsapi_new_instance \
gsapi_set_arg_encoding \
gsapi_set_stdio \
inet_ntoa \
mkdtemp \
perror \
pow \
powl \
printf \
rmdir \
sendfile \
sleep \
snprintf \
sqrt \
sscanf \
stat \
stderr \
strchr \
strncasecmp \
strncmp \
strndup \
strrchr \
tolower \
unlink \
])

AC_CONFIG_FILES([src/Makefile
doc/Makefile
extra/Makefile
extra/completion/Makefile
extra/completion/bash/Makefile
extra/completion/zsh/Makefile
Makefile])

AC_OUTPUT
