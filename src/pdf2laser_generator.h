#ifndef __PDF2LASER_GENERATOR_H__
#define __PDF2LASER_GENERATOR_H__ 1

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

static bool generate_raster(FILE *pjl_file, FILE *bitmap_file);
static bool generate_vector(FILE *pjl_file, FILE *vector_file);
static bool generate_pjl(FILE *bitmap_file, FILE *pjl_file, FILE *vector_file);

#ifdef __cplusplus
};
#endif

#endif
