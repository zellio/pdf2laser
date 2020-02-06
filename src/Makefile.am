SUFFIXES = .c .h

AM_LFLAGS =
AM_YFLAGS =

bin_PROGRAMS = pdf2laser

CLEANFILES =

BUILT_SOURCES =

pdf2laser_SOURCES = type_point.c type_vector.c type_raster.c type_print_job.c	\
	type_vector_list.c type_vector_list_config.c pdf2laser_generator.c			\
	pdf2laser_printer.c pdf2laser_cli.c pdf2laser.c

pdf2laser_CFLAGS = -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE -Wall -Wextra -Wpedantic -std=c11 -I/usr/local/include
pdf2laser_LDFLAGS = -L/usr/local/lib -lgs -lc
pdf2laser_LDADD =
