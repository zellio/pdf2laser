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

#define STRING_BUILDER_MAX_LENGTH (102400)

#define string_builder_to_char(x) ((x)->string)

typedef struct string_builder string_builder_t;
struct string_builder {
	char *string;
	size_t length;
	size_t max_length;
	char *ptr;
};

string_builder_t *string_builder_create(void);
uint32_t string_builder_destroy(string_builder_t *self);

string_builder_t *string_builder_erase(string_builder_t *self);
string_builder_t *string_builder_add(string_builder_t *self, const char *format, ...);



#ifdef __cplusplus
};
#endif

#endif
