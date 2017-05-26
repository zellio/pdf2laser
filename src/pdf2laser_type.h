#ifndef __PDF2LASER_TYPE_H__
#define __PDF2LASER_TYPE_H__ 1

#include <inttypes.h>
#include <stdbool.h>

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

typedef struct _vector vector_t;
struct _vector {
	vector_t * next;
	vector_t ** prev;
	int x1;
	int y1;
	int x2;
	int y2;
	int p;
};

typedef struct _vectors vectors_t ;
struct _vectors {
	vector_t *vectors;
};

typedef struct raster raster_t;
struct raster {
	uint32_t resolution;
	raster_mode mode;
	uint32_t speed;
	uint32_t power;
	//uint8_t repeat;
	uint32_t screen_size;
};

typedef struct cut cut_t;
struct cut {
	bool optimize;
	uint32_t frequency;
	uint32_t speed;
	uint32_t power;
};

typedef struct job job_t;
struct job {
	char *name;
	bool focus;
	uint8_t flip;
	uint32_t height;
	uint32_t width;

	raster_t *raster;

	cut_t **cutv;
	uint8_t cutc;
};


#ifdef __cplusplus
};
#endif

#endif
