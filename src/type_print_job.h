#ifndef __PDF2LASER_TYPE_PRINT_JOB_H__
#define __PDF2LASER_TYPE_PRINT_JOB_H__ 1

#include <stdbool.h>                  // for bool
#include <stdint.h>                   // for int32_t, uint32_t, int8_t
#include "type_raster.h"              // for raster_t
#include "type_vector_list.h"         // for vector_list_t
#include "type_vector_list_config.h"  // for vector_list_config_t

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

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

	bool vector_optimize;
	bool vector_fallthrough;

	vector_list_config_t *configs;

	bool debug;
};

print_job_t *print_job_create(void);
print_job_t *print_job_destroy(print_job_t *self);

vector_list_config_t *print_job_clone_last_vector_list_config(print_job_t *self, int32_t red, int32_t green, int32_t blue);
vector_list_config_t *print_job_append_vector_list_config(print_job_t *self, vector_list_config_t *new_config);
vector_list_config_t *print_job_append_new_vector_list_config(print_job_t *self, int32_t red, int32_t green, int32_t blue);

vector_list_config_t *print_job_find_vector_list_config_by_id(print_job_t *self, uint32_t id);
vector_list_config_t *print_job_find_vector_list_config_by_rgb(print_job_t *self, int32_t red, int32_t green, int32_t blue);

char *print_job_inspect(print_job_t *self);
char *print_job_to_string(print_job_t *self);

#ifdef __cplusplus
};
#endif

#endif
