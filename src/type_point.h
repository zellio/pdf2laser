#ifndef __PDF2LASER_TYPE_POINT_H__
#define __PDF2LASER_TYPE_POINT_H__ 1

#include <stdint.h>   // for int32_t

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

point_t *point_create(int32_t x, int32_t y);
point_t *point_destroy(point_t *self);
int32_t point_compare(point_t *self, point_t *other);

#ifdef __cplusplus
};
#endif

#endif
