#ifndef __PDF2LASER_TYPE_H__
#define __PDF2LASER_TYPE_H__ 1

#include <inttypes.h>
#include <stdbool.h>

#include "pdf2laser_vector.h"
#include "pdf2laser_vector_list.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef enum {
	RASTER_MODE_COLOR = 'c',
	RASTER_MODE_GREY_SCALE = 'g',
	RASTER_MODE_MONO = 'm',
	RASTER_MODE_NONE = 'n',
} raster_mode;

typedef struct raster raster_t;
struct raster {
	uint32_t resolution;
	raster_mode mode;
	uint32_t speed;
	uint32_t power;
	uint8_t repeat;
	uint32_t screen_size;
};

typedef struct cut cut_t;
struct cut {
	bool optimize;
	uint32_t frequency;
	uint32_t speed;
	uint32_t power;
};

typedef struct print_job print_job_t;
struct print_job {
	char *name;
	bool focus;
	uint8_t flip;
	uint32_t height;
	uint32_t width;

	raster_t *raster;

	int32_t vector_speed[3];
	int32_t vector_power[3];
	int32_t vector_frequency;
	bool vector_optimize;

	vector_list_t **vectors;

	cut_t **cutv;
	uint8_t cutc;

	bool debug;
};


#ifdef __cplusplus
};
#endif

#endif
