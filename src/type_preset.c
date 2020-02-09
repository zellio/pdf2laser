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

preset_t *preset_create(void)
{
	preset_t *preset = calloc(1, sizeof(preset_t));
	preset->print_job = print_job_create();
	return preset;
}

preset_t *preset_file_destroy(preset_t *self)
{
	if (self == NULL)
		return NULL;

	print_job_destroy(self->print_job);

	free(self);

	return NULL;
}

preset_t *preset_load_merge_raster(preset_t *self, raster_t *raster)
{
    print_job_t *print_job = self->print_job;
    if (print_job->raster == NULL) {
        print_job->raster = raster;
        return self;
    }

    if (raster->mode)
        self->print_job->raster->mode = raster->mode;

    if (raster->power)
        self->print_job->raster->power = raster->power;

    if (raster->resolution)
        self->print_job->raster->resolution = raster->resolution;

    if (raster->screen_size)
        self->print_job->raster->screen_size = raster->screen_size;

    if (raster->speed)
        self->print_job->raster->speed = raster->speed;

    return self;
}

preset_t *preset_load_ini_section_raster(preset_t *self, ini_section_t *section)
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

    preset_load_merge_raster(self, raster);

    if (self->print_job->raster != raster) {
        free(raster);
	}

    return self;
}

preset_t *preset_load_ini_section_vector(preset_t *self, ini_section_t *section)
{
	ini_entry_t *color_entry = ini_section_lookup_entry(section, "color");
    if (color_entry == NULL) {
        exit(-1);
    }

    int64_t vid;
    int32_t rc = sscanf(color_entry->value, "%ld", &vid);
    if (rc != 1)
        exit(-1);

    int32_t red, green, blue;
    vector_list_config_id_to_rgb(vid, &red, &green, &blue);

	vector_list_config_t* config = print_job_find_vector_list_config_by_rgb(self->print_job, red, green, blue);
	if (config == NULL) {
		config = print_job_append_new_vector_list_config(self->print_job, red, green, blue);
	}

	// config_t *config = config_config->config;
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

preset_t *preset_load_ini_section_preset(preset_t *self, ini_section_t *section)
{
	for (ini_entry_t *entry = section->entries; entry != NULL; entry = entry->next) {
		switch (*entry->key) {
		case 'n': {
			self->name = strndup(entry->value, MAX_FIELD_LENGTH);
			break;
		}
		case 'a': {
			if (!strncasecmp(entry->value, "true", MAX_FIELD_LENGTH)) {
				self->print_job->vector_fallthrough = true;
			}
			self->print_job->focus = false;
			break;
		}
		case 'f': {
			if (!strncasecmp(entry->value, "true", MAX_FIELD_LENGTH)) {
				self->print_job->vector_fallthrough = true;
			}
			self->print_job->vector_fallthrough = false;
			break;
		}
		default: {
			// error
		}
		}
	}
	return self;
}


preset_t *preset_load_ini_section(preset_t *self, ini_section_t *section)
{
    switch (section->name[0]) {
    case 'p': {
        return preset_load_ini_section_preset(self, section);
    }
    case 'r': {
        return preset_load_ini_section_raster(self, section);
    }
    case 'v': {
        return preset_load_ini_section_vector(self, section);
    }
    default: {
		// error
    }
    }
	return self;
}

preset_t *preset_load_ini_file(preset_t *self, ini_file_t *file)
{
	for (ini_section_t *section = file->sections; section != NULL; section = section->next) {
		preset_load_ini_section(self, section);
	}
	return self;
}

preset_t *ini_file_to_preset(ini_file_t *file)
{
	preset_t *preset = preset_create();
	return preset_load_ini_file(preset, file);
}
