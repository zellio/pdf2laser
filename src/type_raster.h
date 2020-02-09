#ifndef __PDF2LASER_TYPE_RASTER_H__
#define __PDF2LASER_TYPE_RASTER_H__ 1

#include <stdint.h>  // for int32_t, int8_t, uint32_t

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

raster_t *raster_create(void);
raster_t *raster_destroy(raster_t *raster);

#ifdef __cplusplus
};
#endif

#endif
