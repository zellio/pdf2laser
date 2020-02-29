#include "type_preset_file.h"
#include <fcntl.h>        // for open, O_RDONLY
#include <stdio.h>        // for NULL
#include <stdlib.h>       // for free, calloc
#include <string.h>       // for strndup
#include <sys/mman.h>     // for mmap, munmap, MAP_PRIVATE, PROT_READ
#include <sys/stat.h>     // for fstat, stat
#include <unistd.h>       // for close
#include "config.h"       // for FILENAME_NCHARS
#include "ini_parser.h"   // for ini_file_parse
#include "libgen.h"       // for basename
#include "type_preset.h"  // for preset_create, preset_destroy, preset_t

preset_file_t *preset_file_create(char *path)
{
	preset_file_t *preset_file = calloc(1, sizeof(preset_file_t));
	preset_file->path = strndup(path, FILENAME_NCHARS);

	char *preset_basename = strndup(path, FILENAME_NCHARS);
	preset_file->preset = preset_create(basename(preset_basename));

	int source_fd = open(path, O_RDONLY);

	struct stat stat;
	fstat(source_fd, &stat);

	char *mmap_data = mmap((void*)NULL, stat.st_size, PROT_READ, MAP_PRIVATE, source_fd, 0);
	char *buffer = mmap_data;

	ini_file_parse(buffer, &(preset_file->preset->config));

	free(preset_basename);

	munmap((void*)mmap_data, stat.st_size);
	close(source_fd);

	return preset_file;
}

preset_file_t *preset_file_destroy(preset_file_t *self)
{
	if (self == NULL)
		return NULL;

	free(self->path);

	preset_destroy(self->preset);

	free(self);

	return NULL;
}
