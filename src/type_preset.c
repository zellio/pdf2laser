#include "type_preset.h"
#include <ctype.h>                    // for tolower
#include <stdbool.h>                  // for false, true
#include <stdint.h>                   // for int32_t, int64_t
#include <stdio.h>                    // for NULL, sscanf
#include <stdlib.h>                   // for atoi, exit, free, calloc
#include <string.h>                   // for strndup
#include <strings.h>                  // for strncasecmp
#include "ini_file.h"                 // for ini_entry_t, ini_section_t, MAX_FIELD_LENGTH, ini_file_t
#include "type_print_job.h"           // for print_job_t, print_job_append_new_vector_list_config, print_job_create, print_job_destroy, print_job_find_vector_list_config_by_rgb
#include "type_raster.h"              // for raster_t, raster_create, raster_mode
#include "type_vector_list_config.h"  // for vector_list_config_t, vector_list_config_id_to_rgb
#include "config.h"

preset_t *preset_create(char *name)
{
	preset_t *preset = calloc(1, sizeof(preset_t));
	preset->name = strndup(name, PRESET_NAME_NCHARS);
	preset->config = NULL;
	return preset;
}

preset_t *preset_destroy(preset_t *self)
{
	if (self == NULL)
		return NULL;

	ini_file_destroy(self->config);

	free(self);

	return NULL;
}

static preset_t *preset_load_merge_raster(preset_t *self, print_job_t *print_job, raster_t *raster)
{
    if (print_job->raster == NULL) {
        print_job->raster = raster;
        return self;
    }

    if (raster->mode)
        print_job->raster->mode = raster->mode;

    if (raster->power)
        print_job->raster->power = raster->power;

    if (raster->resolution)
        print_job->raster->resolution = raster->resolution;

    if (raster->screen_size)
        print_job->raster->screen_size = raster->screen_size;

    if (raster->speed)
		print_job->raster->speed = raster->speed;

    return self;
}

static preset_t *preset_load_ini_section_raster(preset_t *self, print_job_t *print_job, ini_section_t *section)
{
    raster_t *raster = raster_create();
    for (ini_entry_t *entry = section->entries; entry != NULL; entry = entry->next) {
        switch (*entry->key) {
        case 'm': { // mode
			raster_mode mode = tolower(*entry->value);
			raster->mode = mode;
			break;
        }
        case 'p': { // power
            raster->power = atoi(entry->value);
			break;
        }
        case 'r': { // resolution
            raster->resolution = atoi(entry->value);
			break;
        }
        case 's': { // screen-size
            raster->screen_size = atoi(entry->value);
			break;
        }
        default: {
            // error
        }
        }
    }

    preset_load_merge_raster(self, print_job, raster);

    if (print_job->raster != raster) {
        free(raster);
	}

    return self;
}

static preset_t *preset_load_ini_section_vector(preset_t *self, print_job_t *print_job, ini_section_t *section)
{
	ini_entry_t *color_entry = ini_section_lookup_entry(section, "color");
    if (color_entry == NULL) {
        exit(-1);
    }

    int64_t vid;
    int32_t rc = sscanf(color_entry->value, "%lx", &vid);
    if (rc != 1)
        exit(-1);

    int32_t red, green, blue;
    vector_list_config_id_to_rgb(vid, &red, &green, &blue);

	vector_list_config_t* config = print_job_find_vector_list_config_by_rgb(print_job, red, green, blue);
	if (config == NULL) {
		config = print_job_append_new_vector_list_config(print_job, red, green, blue);
	}

	for (ini_entry_t *entry = section->entries; entry != NULL; entry = entry->next) {
		switch (*entry->key) {
		case 'c': { // color
			continue;
		}

		case 'f': { // frequency
			config->frequency = atoi(entry->value);
			break;
		}
		case 's': { // speed
			config->speed = atoi(entry->value);
			break;
		}
		case 'p': { // power
			config->power = atoi(entry->value);
			break;
		}
		case 'm': { // multipass
			config->multipass = atoi(entry->value);
			break;
		}
		default: {
			// error
		}
		}
	}

	return self;
}

static preset_t *preset_load_ini_section_preset(preset_t * self, print_job_t *print_job, ini_section_t *section)
{
	for (ini_entry_t *entry = section->entries; entry != NULL; entry = entry->next) {
		switch (*entry->key) {
		case 'n': {
			self->name = strndup(entry->value, MAX_FIELD_LENGTH);
			break;
		}
		case 'a': {
			if (!strncasecmp(entry->value, "true", MAX_FIELD_LENGTH)) {
				print_job->vector_fallthrough = true;
			}
			print_job->focus = false;
			break;
		}
		case 'f': {
			if (!strncasecmp(entry->value, "true", MAX_FIELD_LENGTH)) {
				print_job->vector_fallthrough = true;
			}
			print_job->vector_fallthrough = false;
			break;
		}
		default: {
			// error
		}
		}
	}
	return self;
}


static preset_t *preset_load_ini_section(preset_t * self, print_job_t *print_job, ini_section_t *section)
{
    switch (section->name[0]) {
    case 'p': { // preset
        return preset_load_ini_section_preset(self, print_job, section);
    }
    case 'r': { // raster
        return preset_load_ini_section_raster(self, print_job, section);
    }
    case 'v': { // vector
        return preset_load_ini_section_vector(self, print_job, section);
    }
    default: {
		// error
    }
    }
	return self;
}

static preset_t *preset_load_ini_file(preset_t *self, print_job_t *print_job)
{
	for (ini_section_t *section = self->config->sections; section != NULL; section = section->next) {
		preset_load_ini_section(self, print_job, section);
	}
	return self;
}

preset_t *preset_apply_to_print_job(preset_t *self, print_job_t *print_job)
{
	//
	preset_load_ini_file(self, print_job);
	return self;
}
