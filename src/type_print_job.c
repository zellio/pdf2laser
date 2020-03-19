#include "type_print_job.h"
#include <inttypes.h>                 // for PRIxPTR
#include <stdbool.h>                  // for true, false
#include <stddef.h>                   // for NULL, size_t
#include <stdio.h>                    // for snprintf
#include <stdlib.h>                   // for free, calloc
#include <string.h>                   // for strlen, strndup
#include "config.h"                   // for BED_HEIGHT, BED_WIDTH, DEBUG, DEFAULT_HOST, HOSTNAME_NCHARS
#include "type_raster.h"              // for raster_t, raster_create, raster_destroy
#include "type_vector_list_config.h"  // for vector_list_config_t, vector_list_config_create, vector_list_config_destroy, vector_list_config_rgb_to_id, vector_list_config_shallow_clone, vector_list_config_to_string

print_job_t *print_job_create(void)
{
	print_job_t *print_job = calloc(1, sizeof(print_job_t));
	print_job->raster = raster_create();

	print_job->host = strndup(DEFAULT_HOST, HOSTNAME_NCHARS);
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

	size_t config_count = 0;
	for (vector_list_config_t *config = self->configs; config != NULL; config = config->next) {
		config_count += 1;
	}

	vector_list_config_t *configs[config_count];
	size_t index = 0;
	for (vector_list_config_t *config = self->configs; config != NULL; config = config->next) {
		configs[index] = config;
		index += 1;
	}

	for (index = 0; index < config_count; index += 1) {
		vector_list_config_destroy(configs[index]);
	}

	free(self);
	return NULL;
}

char *print_job_inspect(print_job_t *self)
{
	char *s = calloc(30, sizeof(char));
	snprintf(s, 30, "<PrintJob:0x%016"PRIxPTR">", (uintptr_t)self);
	return s;
}

char *print_job_to_string(print_job_t *self)
{
	const char *print_job_string_header_template = "Job: %s\nRaster: speed=%d power=%d dpi=%d";

	size_t s_len = 1;  // \0
	s_len += snprintf(NULL, 0, print_job_string_header_template, self->name, self->raster->speed, self->raster->power, self->raster->resolution);

	size_t config_count = 0;
	for (vector_list_config_t *config = self->configs; config != NULL; config = config->next)
		config_count += 1;

	// size_t buffer_size = 1;  // '\0'
	char *configs[config_count];
	size_t index = 0;
	for (vector_list_config_t *config = self->configs; config != NULL; config = config->next) {
		configs[index] = vector_list_config_to_string(config);

		s_len += 1;  // '\n'
		// should be safe because we generate this string
		s_len += strlen(configs[index]);
	}

	char *s = calloc(s_len, sizeof(char));
	size_t rc = 0;
	rc += snprintf(s + rc, s_len - rc, print_job_string_header_template, self->name, self->raster->speed, self->raster->power, self->raster->resolution);

	for (size_t index = 0; index < config_count; index++) {
		rc += snprintf(s + rc, s_len - rc, "\n%s", configs[index]);
		free(configs[index]);
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
