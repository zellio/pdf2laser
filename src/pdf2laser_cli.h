#ifndef __PDF2LASER_CLI_H__
#define __PDF2LASER_CLI_H__ 1

#include <ctype.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pdf2laser_type.h"

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

bool optparse(print_job_t *print_job, int32_t argc, char **argv);

#ifdef __cplusplus
};
#endif

#endif
