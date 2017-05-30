#ifndef __PDF2LASER_VECTOR_H__
#define __PDF2LASER_VECTOR_H__ 1

#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef struct point point_t;
struct point {
	int32_t x;
	int32_t y;
};

typedef struct vector vector_t;
struct vector {
	point_t *start;
	point_t *end;

	vector_t *next;
	vector_t *prev;
};

vector_t *vector_create(int32_t start_x, int32_t start_y,
						int32_t end_x, int32_t end_y);

bool vector_destroy(vector_t *self);

int32_t vector_compare(vector_t *self, vector_t *other);

#ifdef __cplusplus
};
#endif

#endif
