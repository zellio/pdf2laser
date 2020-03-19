#ifndef __PDF2LASER_UTIL_H__
#define __PDF2LASER_UTIL_H__ 1

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

int pdf2laser_sendfile(int out_fd, int in_fd);
char *pdf2laser_format_string(char *template, ...);

#ifdef __cplusplus
};
#endif

#endif
