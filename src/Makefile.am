SUFFIXES = .c .h .y .l

AM_LFLAGS =
AM_YFLAGS = -d -Wno-yacc

bin_PROGRAMS = pdf2laser

CLEANFILES = ini_lexer.h ini_lexer.c ini_parser.h ini_parser.c

BUILT_SOURCES = ini_lexer.c ini_parser.h

pdf2laser_SOURCES = ini_file.c ini_lexer.l ini_parser.y type_raster.c       \
	type_point.c type_vector.c type_vector_list.c type_vector_list_config.c \
	type_preset.c type_preset_file.c type_print_job.c pdf2laser_util.c      \
	pdf2laser_generator.c pdf2laser_printer.c pdf2laser_cli.c pdf2laser.c

pdf2laser_CFLAGS = -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE -Wall -Wextra -Wpedantic -std=c11 -I/usr/local/include
pdf2laser_LDFLAGS = -L/usr/local/lib
pdf2laser_LDADD =

MAINTAINERCLEANFILES = Makefile.in
