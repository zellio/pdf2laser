#include "type_vector.h"
#include <stdlib.h>  // for calloc, free
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

vector_t *vector_create(int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y)
{
	vector_t *vector = calloc(1, sizeof(vector_t));

	vector->start = point_create(start_x, start_y);
	vector->end = point_create(end_x, end_y);

	return vector;
}

vector_t *vector_destroy(vector_t *self)
{
	if (self == NULL)
		return NULL;

	point_destroy(self->start);
	point_destroy(self->end);

	free(self);

	return NULL;
}

int32_t vector_compare(vector_t *self, vector_t *other)
{
	int32_t self_x = self->start->x - self->end->x;
	int32_t self_y = self->start->y - self->end->y;
	int32_t self_length = (self_x * self_x + self_y * self_y);
	int32_t self_rads = (int32_t)(180.0 * atan2(self_y, self_x) / M_PI);

	int32_t other_x = other->start->x - other->end->x;
	int32_t other_y = other->start->y - other->end->y;
	int32_t other_length = (other_x * other_x + other_y * other_y);
	int32_t other_rads = (int32_t)(180.0 * atan2(other_y, other_x) / M_PI);

	return self_length + self_rads - other_length - other_rads;
}

vector_t *vector_flip(vector_t *self)
{
	point_t *start = self->start;
	self->start = self->end;
	self->end = start;
	return self;
}
