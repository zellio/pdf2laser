#ifndef __PDF2LASER_CLI_H__
#define __PDF2LASER_CLI_H__ 1

#include <stdbool.h>         // for bool
#include <stdint.h>          // for int32_t
#include "type_print_job.h"  // for print_job_t
#include <stddef.h> // for size_t
#include "config.h"
#include "type_print_job.h"  // for print_job_t
#include "type_preset_file.h" // for preset_file_t

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

#define OPTARG_MAX_LENGTH 1024

bool pdf2laser_optparse(print_job_t *print_job, preset_file_t **preset_files, size_t preset_files_count, int32_t argc, char **argv);

#ifdef __cplusplus
};
#endif

#endif
