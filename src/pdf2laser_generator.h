#ifndef __PDF2LASER_GENERATOR_H__
#define __PDF2LASER_GENERATOR_H__ 1

#include <stdbool.h>         // for bool
#include <stdio.h>           // for FILE
#include "type_print_job.h"  // for print_job_t

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

//Number of bytes in the bitmap header.
#define BITMAP_HEADER_NBYTES (54)

// how many different vector power level groups
#define VECTOR_PASSES 3

int generate_pdf(const char * source_pdf, const char *target_pdf);
int generate_ps(const char *target_pdf, const char *target_ps);
int generate_eps(print_job_t *print_job, char *target_ps_file, char *target_eps_file);
int generate_raster(print_job_t *print_job, FILE *pjl_file, FILE *bitmap_file);
int generate_vector(print_job_t *print_job, FILE *pjl_file, FILE *vector_file);
int generate_pjl(print_job_t *print_job, char *bmp_target, char *vector_target, char *pjl_target);

#ifdef __cplusplus
};
#endif

#endif
