#include "pdf2laser_vector.h"
#include <stdlib.h>  // for calloc, free

point_t *point_create(int32_t x, int32_t y)
{
	point_t *point = calloc(1, sizeof(point_t));

	point->x = x;
	point->y = y;

	return point;
}

bool point_destroy(point_t *self)
{
	if (self == NULL)
		return false;

	free(self);

	return true;
}

int32_t point_compare(point_t *self, point_t *other)
{
	if (self->x > other->x || self->y > other->y)
		return 1;

	if (self->x < other->x || self->y < other->y)
		return -1;

	return 0;
}

vector_t *vector_create(int32_t start_x, int32_t start_y,
						int32_t end_x, int32_t end_y)
{
	vector_t *vector = calloc(1, sizeof(vector_t));

	vector->start = point_create(start_x, start_y);
	vector->end = point_create(end_x, end_y);

	return vector;
}

bool vector_destroy(vector_t *self)
{
	if (self == NULL)
		return false;

	if (self->start != NULL)
		point_destroy(self->start);

	if (self->end != NULL)
		point_destroy(self->end);

	free(self);

	return true;
}

int32_t vector_compare(vector_t *self, vector_t *other)
{
	int32_t ssc = point_compare(self->start, other->start);
	int32_t eec = point_compare(self->end, other->end);
	int32_t sec = point_compare(self->start, other->end);
	int32_t esc = point_compare(self->end, other->start);

	if ((!ssc && !eec) || (!sec && !esc)) {
		return 0;
	}
	else if (ssc > 0 || eec > 0 || sec > 0 || esc > 0) {
		return 1;
	}

	return -1;
}

vector_t *vector_flip(vector_t *self)
{
	point_t *start = self->start;
	self->start = self->end;
	self->end = start;
	return self;
}
