#include "pdf2laser_type.h"
#include "type_vector_list.h"

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

vector_list_config_t *vector_list_config_create(void) {
	vector_list_config_t *vector_list_config = calloc(1, sizeof(vector_list_config_t));
	return vector_list_config;
}

vector_list_config_t *print_job_append_vector_list(print_job_t *self, vector_list_t *vector_list, int32_t red, int32_t green, int32_t blue)
{
	vector_list_config_t *current_config = self->configs;
	vector_list_config_t *new_config = vector_list_config_create();

	new_config->id = vector_list_config_rgb_to_id(red, green, blue);
	new_config->vector_list = vector_list;

	if (current_config == NULL) {
		new_config->index = 0;
		self->configs = new_config;
		return new_config;
	}

	int32_t index = 1;
	while (current_config->next != NULL) {
		index += 1;
		current_config = current_config->next;
	}
	new_config->index = index;
	current_config->next = new_config;

	return new_config;
}

vector_list_config_t *print_job_append_new_vector_list(print_job_t *self, int32_t red, int32_t green, int32_t blue)
{
	vector_list_t *vector_list = vector_list_create();
	return print_job_append_vector_list(self, vector_list, red, green, blue);
}

vector_list_config_t *print_job_clone_last_vector_list(print_job_t *self, int32_t red, int32_t green, int32_t blue)
{
	vector_list_config_t *config = self->configs;
	while (config->next) {
		config = config->next;
	}
	vector_list_t *vector_list = vector_list_shallow_clone(config->vector_list);
	return print_job_append_vector_list(self, vector_list, red, green, blue);
}

vector_list_config_t *print_job_find_vector_list_config_by_id(print_job_t *self, uint32_t id)
{
	for (vector_list_config_t *config = self->configs;
		 config != NULL;
		 config = config->next) {

		if (config->id == id)
			return config;
	}
	return NULL;
}

vector_list_config_t *print_job_find_vector_list_config_by_rgb(print_job_t *self, int32_t red, int32_t green, int32_t blue)
{
	return print_job_find_vector_list_config_by_id(self, vector_list_config_rgb_to_id(red, green, blue));
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

char *print_job_inspect(print_job_t *self)
{
	char *s = calloc(30, sizeof(char));
	snprintf(s, 30, "<PrintJob:0x%016lx>", (uintptr_t)self);
	return s;
}

char *print_job_to_string(print_job_t *self)
{
	int32_t length = 0;
	vector_list_config_t *configs = self->configs;
	while (configs) {
		length += 1;
		configs = configs->next;
	}

	int32_t max_str_size =
		6 + strnlen(self->name, 1024) + 39 + (65 * (int)length) + 1;
	char *s = calloc(max_str_size, sizeof(char));

	snprintf(s, max_str_size,
			 "Job: %s\nRaster: speed=%d power=%d dpi=%d\n",
			 self->name,
			 self->raster->speed,
			 self->raster->power,
			 self->raster->resolution);

	for (vector_list_config_t *config = self->configs;
		 config != NULL;
		 config = config->next) {

		strncat(s, vector_list_config_to_string(config), max_str_size);

		if (config->next)
			strncat(s, "\n", max_str_size);
	}

	return s;
}
