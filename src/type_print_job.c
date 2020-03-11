#include "type_print_job.h"
#include <stdbool.h>                  // for true, false
#include <stdio.h>                    // for NULL, snprintf
#include <stdlib.h>                   // for free, calloc
#include <string.h>                   // for strncat, strndup, strnlen
#include "config.h"                   // for BED_HEIGHT, BED_WIDTH, DEBUG, DEFAULT_HOST
#include "type_raster.h"              // for raster_create, raster_destroy, raster_t
#include "type_vector_list_config.h"  // for vector_list_config_t, vector_list_config_create, vector_list_config_destroy, vector_list_config_rgb_to_id, vector_list_config_shallow_clone, vector_list_config_to_string

print_job_t *print_job_create(void)
{
	print_job_t *print_job = calloc(1, sizeof(print_job_t));
	print_job->raster = raster_create();

	print_job->host = strndup(DEFAULT_HOST, 1024);
	print_job->mode = PRINT_JOB_MODE_COMBINED;
	print_job->height = BED_HEIGHT;
	print_job->width = BED_WIDTH;
	print_job->focus = false;
	print_job->vector_optimize = true;
	print_job->vector_fallthrough = true;
	print_job->configs = NULL;
	print_job->debug = DEBUG;

	return print_job;
}

print_job_t *print_job_destroy(print_job_t *self)
{
	if (self == NULL)
		return NULL;

	free(self->source_filename);
	free(self->host);
	free(self->name);

	raster_destroy(self->raster);

	for (vector_list_config_t *config = self->configs; config != NULL; config = config->next)
		vector_list_config_destroy(config);

	free(self);
	return NULL;
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

vector_list_config_t *print_job_append_vector_list_config(print_job_t *self, vector_list_config_t *new_config)
{
	vector_list_config_t *config = self->configs;

	if (config == NULL) {
		new_config->index = 0;
		self->configs = new_config;
		return new_config;
	}

	int32_t index = 1;
	while (config->next != NULL) {
		index += 1;
		config = config->next;
	}
	new_config->index = index;
	config->next = new_config;

	return new_config;
}

vector_list_config_t *print_job_append_new_vector_list_config(print_job_t *self, int32_t red, int32_t green, int32_t blue)
{
	vector_list_config_t *new_config = vector_list_config_create(red, green, blue);
	return print_job_append_vector_list_config(self, new_config);
}

vector_list_config_t *print_job_clone_last_vector_list_config(print_job_t *self, int32_t red, int32_t green, int32_t blue)
{
	vector_list_config_t *config = self->configs;
	while (config->next) {
		config = config->next;
	}
	vector_list_config_t *config_shallow_clone = vector_list_config_shallow_clone(config, red, green, blue);
	return print_job_append_vector_list_config(self, config_shallow_clone);
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
