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
#include "pdf2laser_generator.h"

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

#define VECTOR_PASSES 3

static bool ps_to_eps(print_job_t *print_job, FILE *ps_file, FILE *eps_file);
static void range_checks(print_job_t *print_job);
static void usage(int rc, const char* const msg);
static int32_t vector_param_set(int32_t **value, const char *optarg);

int main(int argc, char *argv[]);

#ifdef __cplusplus
};
#endif

#endif
