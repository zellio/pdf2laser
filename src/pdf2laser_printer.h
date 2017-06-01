#ifndef __PDF2LASER_PRINTER_H__
#define __PDF2LASER_PRINTER_H__ 1

#include <stdbool.h>  // for bool
#include <stdio.h>    // For FILE

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/** Number of seconds per a minute. */
#define SECONDS_PER_MIN (60)

/** Maximum wait before timing out on connecting to the printer (in seconds). */
#define PRINTER_MAX_WAIT (300)

bool printer_send(const char *host, FILE *pjl_file, const char *job_name);

#ifdef __cplusplus
};
#endif

#endif
