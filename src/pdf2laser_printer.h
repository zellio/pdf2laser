#ifndef __PDF2LASER_PRINTER_H__
#define __PDF2LASER_PRINTER_H__ 1

/* #include <stdbool.h> */
/* #include <stdio.h> */
/* #include <sys/types.h> */
/* #include <sys/socket.h> */
/* #include <netdb.h> */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include <netdb.h>

#include <unistd.h>

#include <sys/socket.h>

#include <arpa/inet.h>

#include <errno.h>

#include <string.h>

#include <stdlib.h>

//#include <netinet/in.h>


#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/** Number of seconds per a minute. */
#define SECONDS_PER_MIN (60)

/** Maximum allowable hostname characters. */
#define HOSTNAME_NCHARS (1024)

/** Maximum wait before timing out on connecting to the printer (in seconds). */
#define PRINTER_MAX_WAIT (300)

static int32_t printer_connect(const char *host, const uint32_t timeout);
static bool printer_disconnect(int32_t socket_descriptor);

bool printer_send(const char *host, FILE *pjl_file);

#ifdef __cplusplus
};
#endif

#endif
