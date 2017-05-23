#include "pdf2laser_string_builder.h"

string_builder_t *string_builder_create(void)
{
	string_builder_t *string_builder = (string_builder_t*)calloc(1, sizeof(string_builder_t));
	if (string_builder == NULL)
		return NULL;

	string_builder->max_length = STRING_BUILDER_MAX_LENGTH;
	string_builder->length = 0;
	string_builder->string = calloc(max_length, sizeof(char));
	string_builder->ptr = string_builder->ptr;

	return string_builder;
}

uint32_t string_builder_destroy(string_builder_t *self)
{
	if (self == NULL)
		return -1;

	if (self->string != NULL)
		free(self->string);

	free(self);

	return 0;
}

string_builder_t *string_builder_erase(string_builder_t *self)
{
	memset(self->string, '\0', self->max_size);
	self->length = 0;
	self->ptr = self->string;
	return self;
}

string_builder_t *string_builder_add(string_builder_t *self, const char *format, ...)
{
	va_list va_args;
	va_start(va_args, format);

	uint64_t wl = vsnprintf(self->ptr, self->max_length - self->length, format, va_args);

	self->length += wl;
	self->ptr += wl;

	va_end(args);

	return self;
}
