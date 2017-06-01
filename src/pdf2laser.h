#ifndef __PDF2LASER_H__
#define __PDF2LASER_H__ 1

#include <stdbool.h> // for bool, true, false

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/* #define VECTOR_PASSES (3) */

/** Default on whether or not auto-focus is enabled. */
#define AUTO_FOCUS (true)

/** Default bed height (y-axis) in pts. */
#define BED_HEIGHT (864)

/** Default bed width (x-axis) in pts. */
#define BED_WIDTH (1728)

/** Default for debug mode. */
#define DEBUG (false)

/** Number of characters allowable for a filename. */
#define FILENAME_NCHARS (1024)

/** default on whether or not the result is supposed to be flipped along the X
 * axis.
 */
#define FLIP (false)

/** Default mode for processing raster engraving (varying power depending upon
 * image characteristics).
 */
#define RASTER_MODE_DEFAULT (RASTER_MODE_MONO)

/** Default power level for raster engraving */
#define RASTER_POWER_DEFAULT (40)

/** Whether or not the raster printing is to be repeated. */
#define RASTER_REPEAT (1)

/** Default speed for raster engraving */
#define RASTER_SPEED_DEFAULT (100)

/** Default resolution is 600 DPI */
#define RESOLUTION_DEFAULT (600)

/** Pixel size of screen (0 is threshold).
 * FIXME - add more details
 */
#define SCREEN_DEFAULT (8)

/** Temporary directory to store files. */
#define TMP_DIRECTORY "/tmp"

/** FIXME */
#define VECTOR_FREQUENCY_DEFAULT (5000)

/* /\** Default power level for vector cutting. *\/ */
/* #define VECTOR_POWER_DEFAULT (50) */

/* /\** Default speed level for vector cutting. *\/ */
/* #define VECTOR_SPEED_DEFAULT (30) */

/* static void range_checks(print_job_t *print_job); */
/* static void usage(int rc, const char* const msg); */

int main(int argc, char *argv[]);

#ifdef __cplusplus
};
#endif

#endif
