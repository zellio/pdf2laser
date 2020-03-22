#ifndef __PDF2LASER_TYPE_VECTOR_LIST_H__
#define __PDF2LASER_TYPE_VECTOR_LIST_H__ 1

#include <stdbool.h>      // for bool
#include <stdint.h>       // for int32_t
#include "type_point.h"   // for point_t
#include "type_vector.h"  // for vector_t

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef struct vector_list vector_list_t;
struct vector_list {
	vector_t *head;
	vector_t *tail;
	int32_t length;
};

vector_list_t *vector_list_create(void);
vector_list_t *vector_list_destroy(vector_list_t *self);

vector_list_t *vector_list_append(vector_list_t *self, vector_t *vector);
vector_t *vector_list_remove(vector_list_t *self, vector_t *vector);
int vector_list_contains(vector_list_t *self, vector_t *vector);

vector_t *vector_list_find_closest(vector_list_t *list, point_t *point);
vector_list_t *vector_list_optimize(vector_list_t *self);

vector_list_t *vector_list_stats(vector_list_t *self);

#ifdef __cplusplus
};
#endif

#endif
