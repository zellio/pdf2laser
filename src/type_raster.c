#include "type_raster.h"
#include <stdlib.h>

raster_t *raster_create(void)
{
	raster_t *raster = calloc(1, sizeof(raster_t));
	return raster;
}

raster_t *raster_destroy(raster_t *raster)
{
	free(raster);
	return NULL;
}
