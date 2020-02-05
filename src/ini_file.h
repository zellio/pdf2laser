#ifndef __INI_FILE_H__
#define __INI_FILE_H__ 1

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef struct ini_entry ini_entry_t;
struct ini_entry {
	char *key;
	char *value;
	ini_entry_t *next;
};

typedef struct ini_section ini_section_t;
struct ini_section {
	char *name;
	ini_entry_t *entries;
	ini_section_t *next;
};

typedef struct ini_file ini_file_t;
typedef ini_file_t config_file_t;
struct ini_file {
	char *path;
	ini_section_t *sections;
};

#define MAX_FIELD_LENGTH 1024
#define MAX_SECTION_LENGTH 1024000

ini_file_t *ini_file_create(char *path);
ini_section_t *ini_section_create(char *name);
ini_entry_t *ini_entry_create(char *key, char *value);

char *ini_entry_to_string(ini_entry_t *self);
char *ini_section_to_string(ini_section_t *self);
char *ini_file_to_string(ini_file_t *self);

ini_section_t *ini_file_lookup_section(ini_file_t *self, char *section_name);

#ifdef __cplusplus
};
#endif

#endif
