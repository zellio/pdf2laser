#ifndef __PDF2LASER_TYPE_PRESET_FILE_H__
#define __PDF2LASER_TYPE_PRESET_FILE_H__ 1

#include "type_preset.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef struct preset_file preset_file_t;
struct preset_file {
    char *path;
	preset_t *preset;
};

preset_file_t *preset_file_create(char *path);
preset_file_t *preset_file_destroy(preset_file_t *self);

#ifdef __cplusplus
};
#endif

#endif
