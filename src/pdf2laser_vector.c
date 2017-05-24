#include "pdf2laser_vector.h"

pair_t *pair_create(int32_t x, int32_t y)
{
	pair_t *pair = calloc(1, sizeof(pair_t));

	pair->x = x;
	pair->y = y;

	return pair;
}

bool pair_destroy(pair_t *self)
{
	if (self == NULL)
		return false;

	free(self);

	return true;
}

int32_t pair_compare(pair_t *self, pair_t *other)
{
	if (self->x > other->x || self->y > other->y)
		return 1;

	if (self->x < other->x || self->y < other->y)
		return -1;

	return 0;
}

vector_t *vector_create(int32_t start_x, int32_t start_y,
						int32_t end_x, int32_t end_y,
						int32_t power)
{
	vector_t *vector = calloc(1, sizeof(vector_t));

	vector->start = pair_create(start_x, start_y);
	vector->end = pair_create(end_x, end_y);
	vector->power = power;

	return vector;
}

bool vector_destroy(vector_t *self)
{
	if (self == NULL)
		return false;

	if (self->start != NULL)
		pair_destroy(self->start);

	if (self->end != NULL)
		pair_destroy(self->end);

	free(self);

	return true;
}

int32_t vector_compare(vector_t *self, vector_t *other)
{
	int32_t ssc = pair_compare(self->start, other->start);
	int32_t eec = pair_compare(self->end, other->end);
	int32_t sec = pair_compare(self->start, other->end);
	int32_t esc = pair_compare(self->end, other->start);

	if ((!ssc && !eec) || (!sec && !esc)) {
		return 0;
	}
	else if (ssc > 0 || eec > 0 || sec > 0 || esc > 0) {
		return 1;
	}

	return -1;
}
