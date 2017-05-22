#ifndef __PDF2LASER_H__
#define __PDF2LASER_H__ 1

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <limits.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>

#include <ghostscript/iapi.h>
#include <ghostscript/ierrors.h>

#include <libgen.h>

#include "pdf2laser_type.h"
#include "pdf2laser_printer.h"

#include "config.h"


#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

static int big_to_little_endian(uint8_t *position, int bytes);
static bool generate_raster(FILE *pjl_file, FILE *bitmap_file);
static bool generate_vector(FILE *pjl_file, FILE *vector_file);
static bool generate_pjl(FILE *bitmap_file, FILE *pjl_file, FILE *vector_file);
static bool ps_to_eps(FILE *ps_file, FILE *eps_file);
static void range_checks(void);
static void usage(int rc, const char* const msg);
static int vector_param_set(int * const values, const char *arg);

int main(int argc, char *argv[]);

#ifdef __cplusplus
};
#endif

#endif
