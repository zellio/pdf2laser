#include "ini_file.h"

#include <stddef.h>   // for NULL, size_t
#include <stdint.h>   // for int32_t
#include <stdio.h>    // for snprintf
#include <stdlib.h>   // for calloc, free
#include <string.h>   // for strnlen, strndup
#include <strings.h>  // for strncasecmp

ini_entry_t *ini_entry_create(char *key, char *value)
{
    ini_entry_t *ini_entry = calloc(1, sizeof(ini_entry_t));
    ini_entry->key = strndup(key, MAX_FIELD_LENGTH);
    ini_entry->value = strndup(value, MAX_FIELD_LENGTH);
    return ini_entry;
}

ini_section_t *ini_section_create(char *name)
{
    ini_section_t *ini_section = calloc(1, sizeof(ini_section_t));
    ini_section->name = strndup(name, MAX_FIELD_LENGTH);
    return ini_section;
}

ini_file_t *ini_file_create(char *path)
{
    ini_file_t *ini_file = calloc(1, sizeof(ini_file_t));
    ini_file->path = strndup(path, MAX_FIELD_LENGTH);
    return ini_file;
}

char *ini_entry_to_string(ini_entry_t *self)
{
    size_t key_length = strnlen(self->key, MAX_FIELD_LENGTH);
    size_t val_length = strnlen(self->value, MAX_FIELD_LENGTH);
    size_t entry_length = key_length + val_length + 1;
    char *s = calloc(entry_length + 1, sizeof(char));
    snprintf(s, entry_length, "%s=%s", self->key, self->value);
    return s;
}

char *ini_section_to_string(ini_section_t *self)
{
    size_t total_buffer_length = 1; // \0
    total_buffer_length += strnlen(self->name, MAX_FIELD_LENGTH) + 2; // [self->name]
    for (ini_entry_t *entry = self->entries; entry != NULL; entry = entry->next) {
        total_buffer_length += 1; // \n
        total_buffer_length += strnlen(entry->key, MAX_FIELD_LENGTH);
        total_buffer_length += 1; // =
        total_buffer_length += strnlen(entry->value, MAX_FIELD_LENGTH);
    }

    char *s = calloc(total_buffer_length, sizeof(char));
    size_t rc = 0;
    rc += snprintf(s + rc, total_buffer_length - rc, "[%s]", self->name);
    for (ini_entry_t *entry = self->entries; entry != NULL; entry = entry->next) {
        rc += snprintf(s + rc, total_buffer_length - rc, "\n%s=%s", entry->key, entry->value);
    }

    return s;
}

char *ini_file_to_string(ini_file_t *self)
{
    int32_t section_count = 0;
    for (ini_section_t *section = self->sections; section != NULL; section = section->next) {
        section_count += 1;
    }

    char *sections[section_count];
    int32_t index = 0;
    size_t buffer_size = 1;
    for (ini_section_t *section = self->sections; section != NULL; section = section->next) {
        sections[index] = ini_section_to_string(section);
        buffer_size += strnlen(sections[index], MAX_SECTION_LENGTH);

        // Place two new lines between sections
        buffer_size += 2;
        index += 1;
    }

    char *s = calloc(buffer_size + 1, sizeof(char));
    int rc = 0;
    for (int index = 0; index < section_count; index += 1) {
        rc += snprintf(s + rc, buffer_size - rc, "%s\n\n", sections[index]);
        free(sections[index]);
    }

    // removes trailing newline from "file"
    s[rc - 1] = '\0';

    return s;
}

ini_section_t *ini_file_find_section(ini_file_t *self, char *name)
{
    for (ini_section_t *section = self->sections; section != NULL; section = section->next) {
        if (!strncasecmp(section->name, name, MAX_FIELD_LENGTH)) {
            return section;
        }
    }
    return NULL;
}
