#ifndef __PDF2LASER_TYPE_VECTOR_H__
#define __PDF2LASER_TYPE_VECTOR_H__ 1

#include <stdint.h>      // for int32_t
#include "type_point.h"  // for point_t

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef struct vector vector_t;
struct vector {
	point_t *start;
	point_t *end;

	vector_t *next;
	vector_t *prev;
};

vector_t *vector_create(int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y);
vector_t *vector_destroy(vector_t *self);
int32_t vector_compare(vector_t *self, vector_t *other);

vector_t *vector_flip(vector_t *self);

#ifdef __cplusplus
};
#endif

#endif
