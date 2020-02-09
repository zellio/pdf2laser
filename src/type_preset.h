#ifndef __PDF2LASER_TYPE_PRESET_H__
#define __PDF2LASER_TYPE_PRESET_H__ 1

#include "type_print_job.h"  // for print_job_t

struct preset;

struct preset;

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef struct preset preset_t;
struct preset {
	char *name;
	print_job_t *print_job;
};

preset_t *preset_create(void);
preset_t *preset_file_destroy(preset_t *self);

#ifdef __cplusplus
};
#endif

#endif
