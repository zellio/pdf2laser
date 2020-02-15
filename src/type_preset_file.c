#include "type_preset_file.h"
#include "config.h"
#include "ini_file.h"
#include "ini_parser.h"
#include "libgen.h"
#include "type_preset.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>  // for calloc, free, NULL
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


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

	ini_file_t *source_ini_file;
	ini_file_parse(buffer, &source_ini_file);

	preset_load_ini_file(preset_file->preset, source_ini_file);

	ini_file_destroy(source_ini_file);

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
