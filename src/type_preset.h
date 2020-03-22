#ifndef __PDF2LASER_TYPE_PRESET_H__
#define __PDF2LASER_TYPE_PRESET_H__ 1

#include "type_print_job.h"  // for print_job_t
#include "ini_file.h"        // for ini_file_t

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef struct preset preset_t;
struct preset {
	char *name;
	ini_file_t *config;
};

preset_t *preset_create(char *name);
preset_t *preset_destroy(preset_t *self);

preset_t *preset_apply_to_print_job(preset_t *self, print_job_t *print_job);

#ifdef __cplusplus
};
#endif

#endif
