#include "type_vector_list_config.h"

#include <stdio.h>             // for snprintf, NULL, size_t
#include <stdlib.h>            // for calloc, free
#include "type_vector_list.h"  // for vector_list_create, vector_list_destroy

vector_list_config_t *vector_list_config_create(int32_t red, int32_t green, int32_t blue)
{
	vector_list_config_t *vector_list_config = calloc(1, sizeof(vector_list_config_t));

	vector_list_config->id = vector_list_config_rgb_to_id(red, green, blue);

	vector_list_config->vector_list = vector_list_create();

	vector_list_config->power = 0;
	vector_list_config->speed = 0;
	vector_list_config->multipass = 1;
	vector_list_config->frequency = 10;

	return vector_list_config;
}

vector_list_config_t *vector_list_config_shallow_clone(vector_list_config_t *self, int32_t red, int32_t green, int32_t blue)
{
	vector_list_config_t *config = vector_list_config_create(red, green, blue);

	config->power = self->power;
	config->speed = self->speed;
	config->multipass = self->multipass;
	config->frequency = self->frequency;

	return config;
}

vector_list_config_t *vector_list_config_destroy(vector_list_config_t *self) {
	if (self != NULL) {
		vector_list_destroy(self->vector_list);
	}

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
	static char *template = "Vector: pass=%d color=%02x%02x%02x speed=%d power=%d multipass=%d frequency=%d";

	int32_t red, green, blue;
	vector_list_config_id_to_rgb(self->id, &red, &green, &blue);

	size_t s_len = 1 + snprintf(NULL, 0, template, self->index, red, green, blue, self->speed, self->power, self->multipass, self->frequency);

	char *s = calloc(s_len, sizeof(char));
	snprintf(s, s_len, template, self->index, red, green, blue, self->speed, self->power, self->multipass, self->frequency);
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
