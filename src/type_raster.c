#include "type_raster.h"
#include <stdlib.h>  // for calloc, free, NULL
#include "config.h"  // for RASTER_MODE_DEFAULT, RASTER_POWER_DEFAULT, RASTER_REPEAT, RASTER_SPEED_DEFAULT, RESOLUTION_DEFAULT, SCREEN_DEFAULT

raster_t *raster_create(void)
{
	raster_t *raster = calloc(1, sizeof(raster_t));

	raster->resolution = RESOLUTION_DEFAULT;
	raster->mode = RASTER_MODE_DEFAULT;
	raster->speed = RASTER_SPEED_DEFAULT;
	raster->power = RASTER_POWER_DEFAULT;
	raster->repeat = RASTER_REPEAT;
	raster->screen_size = SCREEN_DEFAULT;

	return raster;
}

raster_t *raster_destroy(raster_t *raster)
{
	free(raster);
	return NULL;
}
