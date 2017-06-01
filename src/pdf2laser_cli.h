#ifndef __PDF2LASER_CLI_H__
#define __PDF2LASER_CLI_H__ 1

#include <stdint.h>          // for int32_t
#include <stdbool.h>         // for bool
#include "pdf2laser_type.h"  // for print_job_t

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
