/* @file ta10.c @verbatim
 *========================================================================
 * Copyright © 2002-2008 Andrews & Arnold Ltd <info@aaisp.net.uk>
 * Copyright 2008 AS220 Labs <brandon@as220.org>
 * Copyright 2011 Trammell Hudson <hudson@osresearch.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *========================================================================
 *
 *
 * Author: Andrew & Arnold Ltd and Brandon Edens
 * Converted to a command line tool by Trammell Hudson
 *
 * Description:
 * TA10 PDF to laser engraver
 *
 * The Epilog laser engraver comes with a windows printer driver. This works
 * well with Corel Draw, and that is about it. There are other windows
 * applications, like inkscape, but these rasterise the image before sending to
 * the windows printer driver, so there is no way to use them to vector cut!
 *
 * The cups-epilog app is a cups backend, so build and link/copy to
 * /usr/lib/cups/backend/epilog. It allows you to print postscript to the laser
 * and both raster and cut. It works well with inkscape.
 *
 * With this linux driver, vector cutting is recognised by any line or curve in
 * 100% red (1.0 0.0 0.0 setrgbcolor).
 *
 * Create printers using epilog://host/Legend/options where host is the
 * hostname or IP of the epilog engraver. The options are as follows. This
 * allows you to make a printer for each different type of material.
 * af	Auto focus (0=no, 1=yes)
 * r	Resolution 75-1200
 * rs	Raster speed 1-100
 * rp	Raster power 0-100
 * vs	Vector speed 1-100
 * vp	Vector power 1-100
 * vf	Vector frequency 10-5000
 * sc	Photograph screen size in pizels, 0=threshold, +ve=line, -ve=spot, used
 *      in mono mode, default 8.
 * rm	Raster mode mono/grey/colour
 *
 * The mono raster mode uses a line or dot screen on any grey levels or
 * colours. This can be controlled with the sc parameter. The default is 8,
 * which makes a nice fine line screen on 600dpi engraving. At 600/1200 dpi,
 * the image is also lightened to allow for the size of the laser point.
 *
 * The grey raster mode maps the grey level to power level. The power level is
 * scaled to the raster power setting (unlike the windows driver which is
 * always 100% in 3D mode).
 *
 * In colour mode, the primary and secondary colours are processed as separate
 * passes, using the grey level of the colour as a power level. The power level
 * is scaled to the raster power setting. Note that red is 100% red, and non
 * 100% green and blue, etc, so 50% red, 0% green/blue is not counted as red,
 * but counts as "grey". 100% red, and 50% green/blue counts as red, half
 * power. This means you can make distinct raster areas of the page so that you
 * do not waste time moving the head over blank space between them.
 *
 * Epolog cups driver
 * Uses gs to rasterise the postscript input.
 * URI is epilog://host/Legend/options
 * E.g. epilog://host/Legend/rp=100/rs=100/vp=100/vs=10/vf=5000/rm=mono/flip/af
 * Options are as follows, use / to separate :-
 * rp   Raster power
 * rs   Raster speed
 * vp   Vector power
 * vs   Vector speed
 * vf   Vector frequency
 * w    Default image width (pt)
 * h    Default image height (pt)
 * sc   Screen (lpi = res/screen, 0=simple threshold)
 * r    Resolution (dpi)
 * af   Auto focus
 * rm   Raster mode (mono/grey/colour)
 * flip X flip (for reverse cut)
 * Raster modes:-
 * mono Screen applied to grey levels
 * grey Grey levels are power (scaled to raster power setting)
 * colour       Each colour grey/red/green/blue/cyan/magenta/yellow plotted
 * separately, lightness=power
 *
 *
 * Installation:
 * gcc -o epilog `cups-config --cflags` cups-epilog.c `cups-config --libs`
 * http://www.cups.org/documentation.php/api-overview.html
 *
 * Manual testing can be accomplished through execution akin to:
 * $ export DEVICE_URI="epilog://epilog-mini/Legend/rp=100/rs=100/vp=100/vs=10/vf=5000/rm=grey"
 * # ./epilog job user title copies options
 * $ ./epilog 123 jdoe test 1 options < hello-world.ps
 *
 */


/*************************************************************************
 * includes
 */
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>


/*************************************************************************
 * local defines
 */

/** Default on whether or not auto-focus is enabled. */
#define AUTO_FOCUS (1)

/** Default bed height (y-axis) in pts. */
#define BED_HEIGHT (864)

/** Default bed width (x-axis) in pts. */
#define BED_WIDTH (1728)

/** Number of bytes in the bitmap header. */
#define BITMAP_HEADER_NBYTES (54)

/** Default for debug mode. */
#define DEBUG (0)

/** Basename for files generated by the program. */
#define FILE_BASENAME "epilog"

/** Number of characters allowable for a filename. */
#define FILENAME_NCHARS (128)

/** Default on whether or not the result is supposed to be flipped along the X
 * axis.
 */
#define FLIP (0)

/** Maximum allowable hostname characters. */
#define HOSTNAME_NCHARS (1024)

/** Additional offset for the X axis. */
#define HPGLX (0)

/** Additional offset for the Y axis. */
#define HPGLY (0)

/** Whether or not to rotate the incoming PDF 90 degrees clockwise. */
#define PDF_ROTATE_90 (1)

/** Accepted number of points per an inch. */
#define POINTS_PER_INCH (72)

/** Maximum wait before timing out on connecting to the printer (in seconds). */
#define PRINTER_MAX_WAIT (300)

/** Default mode for processing raster engraving (varying power depending upon
 * image characteristics).
 * Possible values are:
 * 'c' = color determines power level
 * 'g' = grey-scale levels determine power level
 * 'm' = mono mode
 * 'n' = no rasterization
 */
#define RASTER_MODE_DEFAULT 'm'

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

/** Number of seconds per a minute. */
#define SECONDS_PER_MIN (60)

/** Temporary directory to store files. */
#define TMP_DIRECTORY "/tmp"

/** FIXME */
#define VECTOR_FREQUENCY_DEFAULT (5000)

/** Default power level for vector cutting. */
#define VECTOR_POWER_DEFAULT (50)

/** Default speed level for vector cutting. */
#define VECTOR_SPEED_DEFAULT (30)


/*************************************************************************
 * local types
 */


/*************************************************************************
 * local variables
 */

/** Temporary buffer for building out strings. */
static char buf[102400];

/** Determines whether or not debug is enabled. */
static char debug = DEBUG;

/** Variable to track auto-focus. */
static int focus = 0;

/** Variable to track whether or not the X axis should be flipped. */
static char flip = FLIP;

/** Height of the image (y-axis). By default this is the bed's height. */
static int height = BED_HEIGHT;

/** Job name for the print. */
static const char *job_name = NULL;

/** User name that submitted the print job. */
static const char *job_user = NULL;

/** Title for the job print. */
static const char *job_title = NULL;

/** Variable to track the resolution of the print. */
static int resolution = RESOLUTION_DEFAULT;

/** Variable to track the mode for rasterization. One of color 'c', or
 * grey-scale 'g', mono 'm', or none 'n'
 */
static char raster_mode = RASTER_MODE_DEFAULT;

/** Variable to track whether or not a rasterization should be repeated. */
static int raster_repeat = RASTER_REPEAT;

/** FIXME -- pixel size of screen, 0= threshold */
static int screen_size = SCREEN_DEFAULT;

/** Options for the printer. */
static char *queue = "";

/** Variable to track the vector speed. */
static int vector_speed = VECTOR_SPEED_DEFAULT;

/** Variable to track the vector power. */
static int vector_power = VECTOR_POWER_DEFAULT;

/** Variable to track the vector frequency. FIXME */
static int vector_freq = VECTOR_FREQUENCY_DEFAULT;

/** Width of the image (x-axis). By default this is the bed's width. */
static int width = BED_WIDTH;            // default bed

/** X re-center (0 = not). */
static int x_center;

/** Track whether or not to repeat X. */
static int x_repeat = 1;

/** Y re-center (0 = not). */
static int y_center;

/** Track whether or not to repeat X. */
static int y_repeat = 1;

/** Should the vector cutting be optimized and dupes removed? */
static int do_vector_optimize = 1;


/*************************************************************************
 * local functions
 */
static int big_to_little_endian(uint8_t *position, int bytes);
static bool generate_vector(FILE *pjl_file, FILE *vector_file);
static bool generate_pjl(FILE *pjl_file, FILE *vector_file);
static bool ps_to_eps(FILE *ps_file, FILE *eps_file);
static void range_checks(void);
static int printer_connect(const char *host, const int timeout);
static bool printer_disconnect(int socket_descriptor);
static bool printer_send(const char *host, FILE *pjl_file);


/*************************************************************************/

/**
 * Convert a big endian value stored in the array starting at the given pointer
 * position to its little endian value.
 *
 * @param position the starting location for the conversion. Each successive
 * unsigned byte is upto nbytes is considered part of the value.
 * @param nbytes the number of successive bytes to convert.
 *
 * @return An integer containing the little endian value of the successive
 * bytes.
 */
static int
big_to_little_endian(uint8_t *position, int nbytes)
{
    int i;
    int result = 0;

    for (i = 0; i < nbytes; i++) {
        result += *(position + i) << (8 * i);
    }
    return result;
}

/**
 * Execute ghostscript feeding it an ecapsulated postscript file which is then
 * converted into a bitmap image. As a byproduct output of the ghostscript
 * process is redirected to a .vector file which will contain instructions on
 * how to perform a vector cut of lines within the postscript.
 *
 * @param filename_bitmap the filename to use for the resulting bitmap file.
 * @param filename_eps the filename to read in encapsulated postscript from.
 * @param filename_vector the filename that will contain the vector
 * information.
 * @param bmp_mode a string which is one of bmp16m, bmpgray, or bmpmono.
 * @param resolution the encapsulated postscript resolution.
 *
 * @return Return true if the execution of ghostscript succeeds, false
 * otherwise.
 */
static bool
execute_ghostscript(
	const char * const filename_bitmap,
	const char * const filename_eps,
	const char * const filename_vector,
	const char * const bmp_mode,
	int resolution
)
{
	char buf[8192];
	snprintf(buf, sizeof(buf),
		"/opt/local/bin/gs"
			" -q"
			" -dBATCH"
			" -dNOPAUSE"
			" -r%d"
			" -sDEVICE=%s"
			" -sOutputFile=%s"
			" %s"
			" > %s"
			"",
		resolution,
		bmp_mode,
		filename_bitmap,
		filename_eps,
		filename_vector
	);

	if (debug)
		fprintf(stderr, "Executing: %s\n", buf);

	if (system(buf))
		return false;

	return true;
}


typedef struct _vector vector_t;
struct _vector
{
	vector_t * next;
	vector_t ** prev;
	int x1;
	int y1;
	int x2;
	int y2;
	int p;
};


typedef struct
{
	vector_t * vectors;
} vectors_t;


static void
vector_stats(
	vector_t * v
)
{
	int lx = 0;
	int ly = 0;
	long cut_len_sum = 0;
	int cuts = 0;

	long transit_len_sum = 0;
	int transits = 0;

	while (v)
	{
		long t_dx = lx - v->x1;
		long t_dy = ly - v->y1;

		long transit_len = sqrt(t_dx * t_dx + t_dy * t_dy);
		if (transit_len != 0)
		{
			transits++;
			transit_len_sum += transit_len;

			if (0)
			fprintf(stderr, "mov %8u %8u -> %8u %8u\n",
				lx, ly,
				v->x1, v->y1
			);
		}

		long c_dx = v->x1 - v->x2;
		long c_dy = v->y1 - v->y2;

		long cut_len = sqrt(c_dx*c_dx + c_dy*c_dy);
		if (cut_len != 0)
		{
			cuts++;
			cut_len_sum += cut_len;

			if (0)
			fprintf(stderr, "cut %8u %8u -> %8u %8u\n",
				v->x1, v->y1,
				v->x2, v->y2
			);
		}

		// Advance the point
		lx = v->x2;
		ly = v->y2;
		v = v->next;
	}

	fprintf(stderr, "Cuts: %u len %lu\n", cuts, cut_len_sum);
	fprintf(stderr, "Move: %u len %lu\n", transits, transit_len_sum);
}


static void
vector_create(
	vectors_t * const vectors,
	int power,
	int x1,
	int y1,
	int x2,
	int y2
)
{
	// Find the end of the list and, if vector optimization is
	// turned on, check for duplicates
	vector_t ** iter = &vectors->vectors;
	while (*iter)
	{
		vector_t * const p = *iter;

		if (do_vector_optimize)
		{
			if (p->x1 == x1 && p->y1 == y1
			&&  p->x2 == x2 && p->y2 == y2)
				return;
			if (p->x1 == x2 && p->y1 == y2
			&&  p->x2 == x1 && p->y2 == y1)
				return;
			if (x1 == x2
			&&  y1 == y2)
				return;
		}

		iter = &p->next;
	}

	vector_t * const v = calloc(1, sizeof(*v));
	if (!v)
		return;

	v->p = power;
	v->x1 = x1;
	v->y1 = y1;
	v->x2 = x2;
	v->y2 = y2;

	// Append it to the now known end of the list
	v->next = NULL;
	v->prev = iter;
	*iter = v;
}



/**
 * Generate a list of vectors.
 *
 * The vector format is:
 * Pp -- Power setting up to 100
 * Mx,y -- Move (start a line at x,y)
 * Lx,y -- Line to x,y from the current position
 * C -- Closing line segment to the starting position
 * X -- end of file
 *
 * Multi segment vectors are split into individual vectors, which are
 * then passed into the topological sort routine.
 *
 * Exact duplictes will be deleted to try to avoid double hits..
 */
static vectors_t *
vectors_parse(
	FILE * const vector_file
)
{
	vectors_t * const vectors = calloc(1, sizeof(*vectors));
	int mx = 0, my = 0;
	int lx = 0, ly = 0;
	int power = 100;
	int count = 0;

	char buf[256];

	while (fgets(buf, sizeof(buf), vector_file))
	{
		//fprintf(stderr, "read '%s'\n", buf);
		const char cmd = buf[0];
		int x, y;

		switch (cmd)
		{
		case 'P':
			sscanf(buf+1, "%d", &power);
			break;
		case 'M':
			// Start a new line.
			// This also implicitly sets the
			// current laser position
			sscanf(buf+1, "%d,%d", &mx, &my);
			lx = mx;
			ly = my;
			break;
		case 'L':
			// Add a line segment from the current
			// point to the new point, and update
			// the current point to the new point.
			sscanf(buf+1, "%d,%d", &x, &y);
			vector_create(vectors, power, lx, ly, x, y);
			count++;
			lx = x;
			ly = y;
			break;
		case 'C':
			// Closing segment from the current point
			// back to the starting point
			vector_create(vectors, power, lx, ly, mx, my);
			lx = mx;
			lx = my;
			break;
		case 'X':
			goto done;
		default:
			fprintf(stderr, "Unknown command '%c'", cmd);
			return NULL;
		}
	}

done:
	fprintf(stderr, "read %u segments\n", count);
	vector_stats(vectors->vectors);

	return vectors;
}


/** Find the closest vector to a given point and remove it from the list.
 *
 * This might reverse a vector if it is closest to draw it in reverse
 * order.
 */
static vector_t *
vector_find_closest(
	vector_t * v,
	const int cx,
	const int cy
)
{
	long best_dist = LONG_MAX;
	vector_t * best = NULL;
	int do_reverse = 0;

	while (v)
	{
		long dx1 = cx - v->x1;
		long dy1 = cy - v->y1;
		long dist1 = dx1*dx1 + dy1*dy1;

		if (dist1 < best_dist)
		{
			best = v;
			best_dist = dist1;
			do_reverse = 0;
		}

		long dx2 = cx - v->x2;
		long dy2 = cy - v->y2;
		long dist2 = dx2*dx2 + dy2*dy2;
		if (dist2 < best_dist)
		{
			best = v;
			best_dist = dist2;
			do_reverse = 1;
		}

		v = v->next;
	}

	if (!best)
		return NULL;

	// Remove it from the list
	*best->prev = best->next;
	if (best->next)
		best->next->prev = best->prev;

	// If reversing is required, flip the x1/x2 and y1/y2
	if (do_reverse)
	{
		int x1 = best->x1;
		int y1 = best->y1;
		best->x1 = best->x2;
		best->y1 = best->y2;
		best->x2 = x1;
		best->y2 = y1;
	}

	best->next = NULL;
	best->prev = NULL;

	return best;
}


/**
 * Optimize the cut order to minimize transit time.
 *
 * Simplistic greedy algorithm: look for the closest vector that starts
 * or ends at the same point as the current point. 
 *
 * This does not split vectors.
 */
static int
vector_optimize(
	vectors_t * const vectors
)
{
	int cx = 0;
	int cy = 0;

	vector_t * vs = NULL;
	vector_t * vs_tail = NULL;

	while (vectors->vectors)
	{
		vector_t * v = vector_find_closest(vectors->vectors, cx, cy);

		if (!vs)
		{
			// Nothing on the list yet
			vs = vs_tail = v;
		} else {
			// Add it to the tail of the list
			v->next = NULL;
			v->prev = &vs_tail->next;
			vs_tail->next = v;
			vs_tail = v;
		}
		
		// Move the current point to the end of the line segment
		cx = v->x2;
		cy = v->y2;
	}

	vector_stats(vs);

	// Now replace the list in the vectors object with this new one
	vectors->vectors = vs;
	if (vs)
		vs->prev = &vectors->vectors;

	return 0;
}

				
static bool
generate_vector(FILE *pjl_file, FILE *vector_file)
{
	vectors_t * const vectors = vectors_parse(vector_file);
	if (do_vector_optimize)
		vector_optimize(vectors);

	int lx = 0;
	int ly = 0;

	fprintf(pjl_file, ":83\r\n");
	fprintf(pjl_file, ":32\r\n");
	fprintf(pjl_file, ":520,20\r\n");
	fprintf(pjl_file, ":E3500\r\n");
	fprintf(pjl_file, ":P2\r\n"); // use pen 2
	fprintf(pjl_file, ":U0,0\r\n");
	fprintf(pjl_file, ":715\r\n");

	// \note: step and repeat is no longer supported

	const vector_t * v = vectors->vectors;
	while (v)
	{
		if (v->x1 != lx || v->y1 != ly)
		{
			// Stop the laser; we need to transit
			// and then start the laser as we go to
			// the next point.  Note initial ";"
			fprintf(pjl_file, "U%d,%d\r\nD%d,%d\r\n",
				v->x1,
				v->y1,
				v->x2,
				v->y2
			);
		} else {
			// This is the continuation of a line, so
			// just add additional points
			fprintf(pjl_file, "D%d,%d\r\n",
				v->x2,
				v->y2
			);
		}

		// Changing power on the fly is not supported for now
		// \todo: Check v->power and adjust ZS, XR, etc

		// Move to the next vector, updating our current point
		lx = v->x2;
		ly = v->y2;
		v = v->next;
	}

	// Stop the laser (note initial ";")
	fprintf(pjl_file, "\r\nU0,0\r\n");

	return true;
}


/**
 *
 */
static bool
generate_pjl(FILE *pjl_file, FILE *vector_file)
{
    /* We're going to perform a vector print. */
    generate_vector(pjl_file, vector_file);

    return true;
}


/**
 * Convert the given postscript file (ps) converting it to an encapsulated
 * postscript file (eps).
 *
 * @param ps_file a file handle pointing to an opened postscript file that
 * is to be converted.
 * @param eps_file a file handle pointing to the opened encapsulated
 * postscript file to store the result.
 *
 * @return Return true if the function completes its task, false otherwise.
 */
static bool
ps_to_eps(FILE *ps_file, FILE *eps_file)
{
    int xoffset = 0;
    int yoffset = 0;

    int l;
    while (fgets((char *)buf, sizeof (buf), ps_file)) {
        fprintf(eps_file, "%s", (char *)buf);
        if (*buf != '%') {
            break;
        }
        if (!strncasecmp((char *) buf, "%%PageBoundingBox:", 18)) {
            int lower_left_x;
            int lower_left_y;
            int upper_right_x;
            int upper_right_y;
            if (sscanf((char *)buf + 14, "%d %d %d %d",
                       &lower_left_x,
                       &lower_left_y,
                       &upper_right_x,
                       &upper_right_y) == 4) {
                xoffset = lower_left_x;
                yoffset = lower_left_y;
                width = (upper_right_x - lower_left_x);
                height = (upper_right_y - lower_left_y);
                fprintf(eps_file, "/setpagedevice{pop}def\n"); // use bbox
                if (xoffset || yoffset) {
                    fprintf(eps_file, "%d %d translate\n", -xoffset, -yoffset);
                }
                if (flip) {
                    fprintf(eps_file, "%d 0 translate -1 1 scale\n", width);
                }
            }
        }
        if (!strncasecmp((char *) buf, "%!", 2)) {
            fprintf
                (eps_file,
                 "/==={(        )cvs print}def/stroke{currentrgbcolor 0.0 \
eq exch 0.0 eq and exch 0.0 ne and{(P)=== currentrgbcolor pop pop 100 mul \
round  cvi = flattenpath{transform(M)=== round cvi ===(,)=== round cvi \
=}{transform(L)=== round cvi ===(,)=== round cvi =}{}{(C)=}pathforall \
newpath}{stroke}ifelse}bind def/showpage{(X)= showpage}bind def\n");
            if (raster_mode != 'c' && raster_mode != 'g') {
                if (screen_size == 0) {
                    fprintf(eps_file, "{0.5 ge{1}{0}ifelse}settransfer\n");
                } else {
                    int s = screen_size;
                    if (resolution >= 600) {
                        // adjust for overprint
                        fprintf(eps_file,
                                "{dup 0 ne{%d %d div add}if}settransfer\n",
                                resolution / 600, s);
                    }
                    fprintf(eps_file, "%d 30{%s}setscreen\n", resolution / s,
                            (screen_size > 0) ? "pop abs 1 exch sub" :
                            "180 mul cos exch 180 mul cos add 2 div");
                }
            }
        }
    }
    while ((l = fread ((char *) buf, 1, sizeof (buf), ps_file)) > 0) {
        fwrite ((char *) buf, 1, l, eps_file);
    }
    return true;
}



/**
 * Perform range validation checks on the major global variables to ensure
 * their values are sane. If values are outside accepted tolerances then modify
 * them to be the correct value.
 *
 * @return Nothing
 */
static void
range_checks(void)
{
    if (resolution > 1200)
        resolution = 1200;
    else
    if (resolution < 75)
        resolution = 75;

    if (screen_size < 1)
        screen_size = 1;

    if (vector_freq < 10)
        vector_freq = 10;
    else
    if (vector_freq > 5000)
        vector_freq = 5000;

    if (vector_power > 100)
        vector_power = 100;
    else
    if (vector_power < 0)
        vector_power = 0;

    if (vector_speed > 100)
        vector_speed = 100;
    else
    if (vector_speed < 1)
        vector_speed = 1;
}

/**
 * Connect to a printer.
 *
 * @param host The hostname or IP address of the printer to connect to.
 * @param timeout The number of seconds to wait before timing out on the
 * connect operation.
 * @return A socket descriptor to the printer.
 */
static int
printer_connect(const char *host, const int timeout)
{
    int socket_descriptor = -1;
    int i;

    for (i = 0; i < timeout; i++) {
        struct addrinfo *res;
        struct addrinfo *addr;
        struct addrinfo base = { 0, PF_UNSPEC, SOCK_STREAM };
        int error_code = getaddrinfo(host, "printer", &base, &res);

        /* Set an alarm to go off if the program has gone out to lunch. */
        alarm(SECONDS_PER_MIN);

        /* If getaddrinfo did not return an error code then we attempt to
         * connect to the printer and establish a socket.
         */
        if (!error_code) {
            for (addr = res; addr; addr = addr->ai_next) {
		const struct sockaddr_in * addr_in = (void*) addr->ai_addr;
		if (addr_in)
		fprintf(stderr, "trying to connect to %s:%d\n",
			inet_ntoa(addr_in->sin_addr),
			ntohs(addr_in->sin_port)
		);

                socket_descriptor = socket(addr->ai_family, addr->ai_socktype,
                                           addr->ai_protocol);
                if (socket_descriptor >= 0) {
                    if (!connect(socket_descriptor, addr->ai_addr,
                                 addr->ai_addrlen)) {
                        break;
                    } else {
                        close(socket_descriptor);
                        socket_descriptor = -1;
                    }
                }
            }
            freeaddrinfo(res);
        }
        if (socket_descriptor >= 0) {
            break;
        }

        /* Sleep for a second then try again. */
        sleep(1);
    }
    if (i >= timeout) {
        fprintf(stderr, "Cannot connect to %s\n", host);
        return -1;
    }
    /* Disable the timeout alarm. */
    alarm(0);
    /* Return the newly opened socket descriptor */
    return socket_descriptor;
}

/**
 * Disconnect from a printer.
 *
 * @param socket_descriptor the descriptor of the printer that is to be
 * disconnected from.
 * @return True if the printer connection was successfully closed, false otherwise.
 */
static bool
printer_disconnect(int socket_descriptor)
{
    int error_code;
    error_code = close(socket_descriptor);
    /* Report on possible errors to standard error. */
    if (error_code == -1) {
        switch (errno) {
        case EBADF:
            perror("Socket descriptor given was not valid.");
            break;
        case EINTR:
            perror("Closing socket descriptor was interrupted by a signal.");
            break;
        case EIO:
            perror("I/O error occurred during closing of socket descriptor.");
            break;
        }
    }

    /* Return status of disconnect operation to the calling function. */
    if (error_code) {
        return false;
    } else {
        return true;
    }
}


static void usage(int rc, const char * const msg)
{
	static const char usage_str[] =
"Usage: ta10 [options] < file.ps > file.ta10\n"
"Options:\n"
" -P | --preset name                 Select a default preset\n"
"\n"
"Vector options:\n"
" -V | --vector-power 0-100          Vector power\n"
" -v | --vector-speed 0-100          Vector speed\n"
" -O | --no-optimize                 Disable vector optimization\n"
"";
	fprintf(stderr, "%s%s\n", msg, usage_str);
	exit(rc);
}

static const struct option long_options[] = {
	{ "debug",		no_argument, NULL, 'D' },
	{ "preset",		required_argument, NULL, 'P' },
	{ "vector-power",	required_argument, NULL, 'V' },
	{ "vector-speed",	required_argument, NULL, 'v' },
	{ "no-optimize",	no_argument, NULL, 'O' },
	{ NULL, 0, NULL, 0 },
};


/**
 * Main entry point for the program.
 *
 * @param argc The number of command line options passed to the program.
 * @param argv An array of strings where each string represents a command line
 * argument.
 * @return An integer where 0 represents successful termination, any other
 * value represents an error code.
 */
int
main(int argc, char *argv[])
{
	const char * host = "192.168.1.4";

	while (1)
	{
		const char ch = getopt_long(
			argc,
			argv,
			"Dp:P:n:d:r:R:v:V:m:f:s:aO",
			long_options,
			NULL
		);
		if (ch <= 0 )
			break;
		switch(ch)
		{
		case 'D': debug++; break;
		case 'p': host = optarg; break;
		case 'P': usage(EXIT_FAILURE, "Presets are not supported yet\n"); break;
		case 'n': job_name = optarg; break;
		case 'd': resolution = atoi(optarg); break;
		case 'v': vector_speed = atoi(optarg); break;
		case 'V': vector_power = atoi(optarg); break;
		case 'm': raster_mode = tolower(*optarg); break;
		case 'f': vector_freq = atoi(optarg); break;
		case 's': screen_size = atoi(optarg); break;
		case 'a': focus = AUTO_FOCUS; break;
		case 'O': do_vector_optimize = 0; break;
		default: usage(EXIT_FAILURE, "Unknown argument\n"); break;
		}
	}

	/* Perform a check over the global values to ensure that they have values
	 * that are within a tolerated range.
	 */
	range_checks();

	if (!host)
		usage(EXIT_FAILURE, "Printer host must be specfied\n");

	// Skip any of the processed arguments
	argc -= optind;
	argv += optind;

	// If there is an argument after, there must be only one
	// and it will be the input postcript / pdf
	if (argc > 1)
		usage(EXIT_FAILURE, "Only one input file may be specified\n");

	const char * const filename = argc ? argv[0] : "stdin";

	/* Gather the postscript file from either standard input or a filename
	 * specified as a command line argument.
	 */
	FILE * file_cups = argc ? fopen(filename, "r") : stdin;
	if (!file_cups)
	{
		perror(filename);
		exit(EXIT_FAILURE);
	}

	// Report the settings on stdout
	fprintf(stderr,
		"Job: %s (%s)\n"
		"Vector: speed=%d power=%d freq=%d\n"
		"",
		job_title,
		job_user,
		vector_speed,
		vector_power,
		vector_freq
	);


    /* Strings designating filenames. */
    char file_basename[FILENAME_NCHARS];
    char filename_bitmap[FILENAME_NCHARS];
    char filename_eps[FILENAME_NCHARS];
    char filename_pdf[FILENAME_NCHARS];
    char filename_pjl[FILENAME_NCHARS];
    char filename_ps[FILENAME_NCHARS];
    char filename_vector[FILENAME_NCHARS];

    /* Temporary variables. */
    int l;

    /* Determine and set the names of all files that will be manipulated by the
     * program.
     */
    sprintf(file_basename, "%s/%s-%d", TMP_DIRECTORY, FILE_BASENAME, getpid());
    sprintf(filename_bitmap, "%s.bmp", file_basename);
    sprintf(filename_eps, "%s.eps", file_basename);
    sprintf(filename_pjl, "%s.pjl", file_basename);
    sprintf(filename_vector, "%s.vector", file_basename);

    /* File handles. */
    FILE *file_bitmap;
    FILE *file_pdf;
    FILE *file_ps;
    FILE *file_pjl;
    FILE *file_vector;


    /* Check whether the incoming data is ps or pdf data. */
    fread((char *)buf, 1, 4, file_cups);
    rewind(file_cups);
    if (strncasecmp((char *)buf, "%PDF", 4) == 0) {
        /* We have a pdf file. */

        /* Setup the filename for the output pdf file. */
        sprintf(filename_pdf, "%s.pdf", file_basename);

        /* Open the destination pdf file. */
        file_pdf = fopen(filename_pdf, "w");
        if (!file_pdf) {
            perror(filename_pdf);
            return 1;
        }

        /* Write the cups data out to the file_pdf. */
        while ((l = fread((char *)buf, 1, sizeof(buf), file_cups)) > 0) {
            fwrite((char *)buf, 1, l, file_pdf);
        }

        fclose(file_cups);
        fclose(file_pdf);

        /* Setup the postscript output filename. */
        sprintf(filename_ps, "%s.ps", file_basename);

        /* Execute the command pdf2ps to convert the pdf file to ps. */
        sprintf(buf, "/opt/local/bin/pdf2ps %s %s", filename_pdf, filename_ps);
        if (debug) {
            fprintf(stderr, "%s\n", buf);
        }
        if (system(buf)) {
            fprintf(stderr, "Failure to execute pdf2ps. Quitting...");
            return 1;
        }

        if (!debug) {
            /* Debug is disabled so remove generated pdf file. */
            if (unlink(filename_pdf)) {
                perror(filename_pdf);
            }
        }

        /* Set file_ps to the generated ps file. */
        file_ps  = fopen(filename_ps, "r");
    } else {
        /* Input file is postscript. Set the file_ps handle to file_cups. */
        file_ps = file_cups;
    }

    /* Open the encapsulated postscript file for writing. */
    FILE * const file_eps = fopen(filename_eps, "w");
    if (!file_eps) {
        perror(filename_eps);
        return 1;
    }
    /* Convert postscript to encapsulated postscript. */
    if (!ps_to_eps(file_ps, file_eps)) {
        perror("Error converting postscript to encapsulated postscript.");
        fclose(file_eps);
        return 1;
    }

    /* Cleanup after encapsulated postscript creation. */
    fclose(file_eps);
    if (file_ps != stdin) {
        fclose(file_ps);
        if (unlink(filename_ps)) {
            perror(filename_ps);
        }
    }

	const char * const raster_string =
		raster_mode == 'c' ? "bmp16m" :
		raster_mode == 'g' ? "bmpgray" :
		"bmpmono";

    	if(!execute_ghostscript(
		filename_bitmap,
		filename_eps,
		filename_vector,
		raster_string,
		resolution
	)) {
		perror("Failure to execute ghostscript command.\n");
		return 1;
	}

    /* Open file handles needed by generation of the printer job language
     * file.
     */
    file_vector = fopen(filename_vector, "r");

    /* Execute the generation of the printer job language (pjl) file. */
    if (!generate_pjl(stdout, file_vector)) {
        return 1;
    }
    /* Close open file handles. */
    fclose(file_vector);

    /* Cleanup unneeded files provided that debug mode is disabled. */
    if (!debug) {
        if (unlink(filename_bitmap)) {
            perror(filename_bitmap);
        }
        if (unlink(filename_eps)) {
            perror(filename_eps);
        }
        if (unlink(filename_vector)) {
            perror(filename_vector);
        }
    }

    return 0;
}
