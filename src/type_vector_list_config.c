#include "type_vector_list_config.h"
#include <stdlib.h>
#include <stdio.h>

vector_list_config_t *vector_list_config_create(void) {
	vector_list_config_t *vector_list_config = calloc(1, sizeof(vector_list_config_t));
	return vector_list_config;
}

vector_list_config_t *vector_list_config_destroy(vector_list_config_t *self) {
	free(self);
	return NULL;
}

char *vector_list_config_inspect(vector_list_config_t *self)
{
	char *s = calloc(38, sizeof(char));
	snprintf(s, 38, "<VectorListConfig:0x%016lx>", (uintptr_t)self);
	return s;
}

char *vector_list_config_to_string(vector_list_config_t *self)
{
	int32_t red, green, blue;
	vector_list_config_id_to_rgb(self->id, &red, &green, &blue);

	char *s = calloc(64, sizeof(char));
	snprintf(s,
			 64,
			 "Vector: pass=%d color=%02x%02x%02x speed=%d power=%d multipass=%d",
			 self->index,
			 red, green, blue,
			 self->vector_list->speed,
			 self->vector_list->power, self->vector_list->multipass);
	return s;
}

uint32_t vector_list_config_rgb_to_id(int32_t red, int32_t green, int32_t blue)
{
	return (int32_t)((red << 16) + (green << 8) + (blue << 0));
}

void vector_list_config_id_to_rgb(int32_t id, int32_t *red, int32_t *green, int32_t *blue)
{
	*red = (id & 0xff0000) >> 16;
	*green = (id & 0x00ff00) >> 8;
	*blue = (id & 0x0000ff) >> 0;
}
