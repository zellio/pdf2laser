#ifndef __PDF2LASER_TYPE_VECTOR_LIST_CONFIG_H__
#define __PDF2LASER_TYPE_VECTOR_LIST_CONFIG_H__ 1

#include <stdint.h>            // for int32_t, uint32_t
#include "type_vector_list.h"  // for vector_list_t

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef struct vector_list_config vector_list_config_t;
struct vector_list_config {
	uint32_t id;
	uint32_t index;

	vector_list_t *vector_list;

	int32_t power;
	int32_t speed;
	int32_t multipass;
	int32_t frequency;

	vector_list_config_t *next;
};

vector_list_config_t *vector_list_config_create(int32_t red, int32_t green, int32_t blue);
vector_list_config_t *vector_list_config_shallow_clone(vector_list_config_t *self, int32_t red, int32_t green, int32_t blue);
vector_list_config_t *vector_list_config_destroy(vector_list_config_t *self);

char *vector_list_config_inspect(vector_list_config_t *self);
char *vector_list_config_to_string(vector_list_config_t *self);

uint32_t vector_list_config_rgb_to_id(int32_t red, int32_t green, int32_t blue);
void vector_list_config_id_to_rgb(int32_t id, int32_t *red, int32_t *green, int32_t *blue);

#ifdef __cplusplus
};
#endif

#endif
