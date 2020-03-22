#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

AC_INIT([pdf2laser], [1.0.0], [https://github.com/zellio/pdf2laser/issues], [pdf2laser])
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/pdf2laser.c])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIRS([m4])

# Define configurable defaults.
AC_ARG_VAR([DEBUG], [Print debug messages])
AC_DEFINE_UNQUOTED([DEBUG], [(${DEBUG=false})], [Print debug messages])

AC_ARG_VAR([DEBUG_PARSER], [Print debug messages for the ini parser.])
AC_DEFINE_UNQUOTED([DEBUG_PARSER], [(${DEBUG_PARSER=${DEBUG=false}})], [Print debug messages for the ini parser.])

AC_ARG_VAR([DEFAULT_HOST], [Default host address when -p/--printer is not specified. If not overridden, defaults to the NYC Resistor laser host (192.168.1.4).])
AC_DEFINE_UNQUOTED([DEFAULT_HOST], ["${DEFAULT_HOST=192.168.1.4}"], [Default host address when -p/--printer is not specified.])

AC_ARG_VAR([AUTO_FOCUS], [Default on whether or not auto-focus is enabled.])
AC_DEFINE_UNQUOTED([AUTO_FOCUS], [(${AUTO_FOCUS=true})], [Default on whether or not auto-focus is enabled.])

AC_ARG_VAR([BED_HEIGHT], [Default bed height (y-axis) in pts.])
AC_DEFINE_UNQUOTED([BED_HEIGHT], [(${BED_HEIGHT=864})], [Default bed height (y-axis) in pts.])

AC_ARG_VAR([BED_WIDTH], [Default bed width (x-axis) in pts.])
AC_DEFINE_UNQUOTED([BED_WIDTH], [(${BED_WIDTH=1728})], [Default bed width (x-axis) in pts.])

AC_ARG_VAR([FILENAME_NCHARS], [Number of characters allowable for a filename.])
AC_DEFINE_UNQUOTED([FILENAME_NCHARS], [(${FILENAME_NCHARS=1024})], [Number of characters allowable for a filename.])

AC_ARG_VAR([HOSTNAME_NCHARS], [Number of characters allowable for a hostname.])
AC_DEFINE_UNQUOTED([HOSTNAME_NCHARS], [(${HOSTNAME_NCHARS=1024})], [Number of characters allowable for a hostname.])

AC_ARG_VAR([GS_ARG_NCHARS], [Number of characters allowable for a hostname.])
AC_DEFINE_UNQUOTED([GS_ARG_NCHARS], [(${GS_ARG_NCHARS=1024})], [Number of characters allowable gs args.])

AC_ARG_VAR([FLIP], [default on whether or not the result is supposed to be flipped along the X axis.])
AC_DEFINE_UNQUOTED([FLIP], [(${FLIP=false})], [default on whether or not the result is supposed to be flipped along the X axis.])

AC_ARG_VAR([RASTER_MODE_DEFAULT], [Default mode for processing raster engraving (varying power depending upon image characteristics).])
AC_DEFINE_UNQUOTED([RASTER_MODE_DEFAULT], [(${RASTER_MODE_DEFAULT=RASTER_MODE_MONO})], [Default mode for processing raster engraving (varying power depending upon image characteristics).])

AC_ARG_VAR([RASTER_POWER_DEFAULT], [Default power level for raster engraving])
AC_DEFINE_UNQUOTED([RASTER_POWER_DEFAULT], [(${RASTER_POWER_DEFAULT=0})], [Default power level for raster engraving])

AC_ARG_VAR([RASTER_REPEAT], [Whether or not the raster printing is to be repeated.])
AC_DEFINE_UNQUOTED([RASTER_REPEAT], [(${RASTER_REPEAT=1})], [Whether or not the raster printing is to be repeated.])

AC_ARG_VAR([RASTER_SPEED_DEFAULT], [Default speed for raster engraving])
AC_DEFINE_UNQUOTED([RASTER_SPEED_DEFAULT], [(${RASTER_SPEED_DEFAULT=0})], [Default speed for raster engraving])

AC_ARG_VAR([RESOLUTION_DEFAULT], [Default resolution is 600 DPI])
AC_DEFINE_UNQUOTED([RESOLUTION_DEFAULT], [(${RESOLUTION_DEFAULT=600})], [Default resolution is 600 DPI])

AC_ARG_VAR([SCREEN_DEFAULT], [Pixel size of screen (0 is threshold).])
AC_DEFINE_UNQUOTED([SCREEN_DEFAULT], [(${SCREEN_DEFAULT=8})], [Pixel size of screen (0 is threshold).])

AC_ARG_VAR([TMP_DIRECTORY], [Temporary directory to store files.])
AC_DEFINE_UNQUOTED([TMP_DIRECTORY], ["${TMP_DIRECTORY=/tmp}"], [Temporary directory to store files.])

AC_ARG_VAR([PRESET_NAME_NCHARS], [Number of characters allowable for a preset name.])
AC_DEFINE_UNQUOTED([PRESET_NAME_NCHARS], [(${PRESET_NAME_NCHARS=1024})], [Number of characters allowable for a preset name.])

# Checks for programs.
AC_PROG_CC([clang gcc])
AC_PROG_CXX([clang])
AC_PROG_YACC([bison yacc])
AC_PROG_LEX([flex lex])
AC_PROG_INSTALL
AC_PROG_LN_S

# Checks for libraries.
AC_SEARCH_LIBS([sqrt], [m])
AC_SEARCH_LIBS([gsapi_new_instance], [gs])

AC_USE_SYSTEM_EXTENSIONS

LT_INIT

# Checks for header files.
AC_CHECK_HEADERS([ \
arpa/inet.h \
ctype.h \
errno.h \
fcntl.h \
getopt.h \
ghostscript/gserrors.h \
ghostscript/iapi.h \
libgen.h \
libintl.h \
limits.h \
malloc.h \
math.h \
netdb.h \
netinet/in.h \
stdbool.h \
stddef.h \
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
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Check for types
AC_CHECK_TYPES([ \
ptrdiff_t \
])

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRNLEN

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
gethostname \
gsapi_delete_instance \
gsapi_exit \
gsapi_init_with_args \
gsapi_new_instance \
gsapi_set_arg_encoding \
gsapi_set_stdio \
inet_ntoa \
memset \
mkdtemp \
munmap \
perror \
pow \
powl \
printf \
rmdir \
sendfile \
sleep \
snprintf \
socket \
sqrt \
sscanf \
stat \
stderr \
strchr \
strncasecmp \
strncmp \
strndup \
strnlen \
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
extra/presets/Makefile
Makefile])

AC_OUTPUT
