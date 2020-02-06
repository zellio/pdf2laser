#ifndef __PDF2LASER_TYPE_H__
#define __PDF2LASER_TYPE_H__ 1

#include <stdbool.h>                // for bool
#include <stdint.h>                 // for int32_t, uint32_t, int8_t
#include "type_vector_list.h"       // for vector_list_t
#include <stdio.h>                  // for snprintf
#include <stdlib.h>                 // for calloc
#include <string.h>                 // for strnlen

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef enum {
	RASTER_MODE_COLOR = 'c',      // color determines power level
	RASTER_MODE_GREY_SCALE = 'g', // grey-scale levels determine power level
	RASTER_MODE_MONO = 'm',       // mono mode
	RASTER_MODE_NONE = 'n',       // no rasterization
} raster_mode;

typedef struct raster raster_t;
struct raster {
	uint32_t resolution;
	raster_mode mode;
	int32_t speed;
	int32_t power;
	int8_t repeat;
	int32_t screen_size;
};

typedef struct vector_list_config vector_list_config_t;
struct vector_list_config {
	uint32_t id;
	uint32_t index;

	vector_list_t *vector_list;

	vector_list_config_t *next;
};

typedef struct print_job print_job_t;
struct print_job {
	char *source_filename;
	char *host;

	char *name;
	bool focus;
	//uint8_t flip;
	uint32_t height;
	uint32_t width;

	raster_t *raster;

	int32_t vector_frequency;
	bool vector_optimize;
	bool vector_fallthrough;

	vector_list_config_t *configs;

	bool debug;
};

uint32_t vector_list_config_rgb_to_id(int32_t red, int32_t green, int32_t blue);
void vector_list_config_id_to_rgb(int32_t id, int32_t *red, int32_t *green, int32_t *blue);

vector_list_config_t *vector_list_config_create(void);

vector_list_config_t *print_job_append_vector_list(print_job_t *self, vector_list_t *vector_list, int32_t red, int32_t green, int32_t blue);
vector_list_config_t *print_job_append_new_vector_list(print_job_t *self, int32_t red, int32_t green, int32_t blue);
vector_list_config_t *print_job_clone_last_vector_list(print_job_t *self, int32_t red, int32_t green, int32_t blue);
vector_list_config_t *print_job_find_vector_list_config_by_id(print_job_t *self, uint32_t id);
vector_list_config_t *
print_job_find_vector_list_config_by_rgb(print_job_t *self, int32_t red, int32_t green, int32_t blue);

char *vector_list_config_inspect(vector_list_config_t *self);
char *vector_list_config_to_string(vector_list_config_t *self);

char *print_job_inspect(print_job_t *self);
char *print_job_to_string(print_job_t *self);

#ifdef __cplusplus
};
#endif

#endif
