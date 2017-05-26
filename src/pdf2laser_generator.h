#ifndef __PDF2LASER_GENERATOR_H__
#define __PDF2LASER_GENERATOR_H__ 1

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "pdf2laser_type.h"

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


bool generate_raster(job_t *job, FILE *pjl_file, FILE *bitmap_file);
bool generate_vector(job_t *job, FILE *pjl_file, FILE *vector_file);
bool generate_pjl(job_t *job, FILE *bitmap_file, FILE *pjl_file, FILE *vector_file);

#ifdef __cplusplus
};
#endif

#endif
