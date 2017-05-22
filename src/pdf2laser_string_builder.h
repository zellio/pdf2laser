#ifndef __PDF2LASER_STRING_BUILDER_H__
#define __PDF2LASER_STRING_BUILDER_H__ 1

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef struct string_builder string_builder_t;
struct string_builder {
	char *string;
	uint64_t length;
	uint64_t max_length;
};

string_builder_t *string_builder_create(uint64_t max_length);
uint32_t string_builder_destroy(string_builder_t *self);

string_builder_t *string_builder_add(string_builder_t *self, size_t size, const char *format, ...);


#ifdef __cplusplus
};
#endif

#endif
