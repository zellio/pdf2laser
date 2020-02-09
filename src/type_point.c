#include "type_point.h"
#include <stdint.h>  // for int64_t
#include <stdlib.h>  // for calloc, free
#include <math.h>    // for atan2

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

point_t *point_create(int32_t x, int32_t y)
{
	point_t *point = calloc(1, sizeof(point_t));

	point->x = x;
	point->y = y;

	return point;
}

point_t *point_destroy(point_t *self)
{
	free(self);
	return NULL;
}

int32_t point_compare(point_t *self, point_t *other)
{
	// Compare points based on linear distance and angle
	int32_t self_length = (self->x * self->x + self->y * self->y);
	int32_t other_length = (other->x * other->x + other->y * other->y);
	int32_t self_rads = (int32_t)(180.0 * atan2(self->y, self->x) / M_PI);
	int32_t other_rads = (int32_t)(180.0 * atan2(other->y, other->x) / M_PI);
	return self_length + self_rads - other_length - other_rads;
}
