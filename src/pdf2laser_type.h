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

	vector_list_t **vectors;

	bool debug;
};


#ifdef __cplusplus
};
#endif

#endif
