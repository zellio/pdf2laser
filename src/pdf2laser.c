
/// pdf2laser.c --- tool for printing to Epilog laser cuttpers

// Copyright (C) 2015-2017 Zachary Elliott <contact@zell.io>
// Copyright (C) 2011-2015 Trammell Hudson <hudson@osresearch.net>
// Copyright (C) 2008-2011 AS220 Labs <brandon@as220.org>
// Copyright (C) 2002-2008 Andrews & Arnold Ltd <info@aaisp.net.uk>

// Authors: Andrew & Arnold Ltd <info@aaisp.net.uk>
//          Brandon Edens <brandon@as220.org>
//          Trammell Hudson <hudson@osresearch.net>
//          Zachary Elliott <contact@zell.io>
// URL: https://github.com/zellio/pdf2laser
// Version: 0.3.3

/// Commentary:

//

/// License:

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.

/// Code:

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

/*************************************************************************
 * includes
 */

#include "pdf2laser.h"

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

/** Number of characters allowable for a filename. */
#define FILENAME_NCHARS (1024)

/** Default on whether or not the result is supposed to be flipped along the X
 * axis.
 */
#define FLIP (0)

/** Additional offset for the X axis. */
#define HPGLX (0)

/** Additional offset for the Y axis. */
#define HPGLY (0)

/** Whether or not to rotate the incoming PDF 90 degrees clockwise. */
#define PDF_ROTATE_90 (1)

/** Accepted number of points per an inch. */
#define POINTS_PER_INCH (72)

/** Default mode for processing raster engraving (varying power depending upon
 * image characteristics).
 * Possible values are:
 * 'c' = color determines power level
 * 'g' = grey-scale levels determine power level
 * 'm' = mono mode
 * 'n' = no rasterization
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
static raster_mode print_raster_mode = RASTER_MODE_DEFAULT;

/** Variable to track the raster speed. */
static int raster_speed = RASTER_SPEED_DEFAULT;

/** Variable to track the raster power. */
static int raster_power = RASTER_POWER_DEFAULT;

/** Variable to track whether or not a rasterization should be repeated. */
static int raster_repeat = RASTER_REPEAT;

/** FIXME -- pixel size of screen, 0= threshold */
static int screen_size = SCREEN_DEFAULT;

// how many different vector power level groups
#define VECTOR_PASSES 3

/** Variable to track the vector speed. */
static int vector_speed[VECTOR_PASSES] = { 100, 100, 100 };

/** Variable to track the vector power. */
static int vector_power[VECTOR_PASSES] = { 1, 1, 1 };

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


/*************************************************************************/


FILE *fn_vector;
static int GSDLLCALL gsdll_stdout(void *minst, const char *str, int len)
{
	fprintf(fn_vector, "%s", str);
	return len;
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
static bool execute_ghostscript(const char * const filename_bitmap,
								const char * const filename_eps,
								const char * const filename_vector,
								const char * const bmp_mode,
								int resolution)
{

	int gs_argc = 8;
	char *gs_argv[8];

	gs_argv[0] = "gs";
	gs_argv[1] = "-q";
	gs_argv[2] = "-dBATCH";
	gs_argv[3] = "-dNOPAUSE";

	gs_argv[4] = calloc(64, sizeof(char));
	snprintf(gs_argv[4], 64, "-r%d", resolution);

	gs_argv[5] = calloc(1024, sizeof(char));
	snprintf(gs_argv[5], 1024, "-sDEVICE=%s", bmp_mode);

	gs_argv[6] = calloc(1024, sizeof(char));
	snprintf(gs_argv[6], 1024, "-sOutputFile=%s", filename_bitmap);

	gs_argv[7] = calloc(1024, sizeof(char));
	snprintf(gs_argv[7], 1024, "%s", filename_eps);

	fn_vector = fopen(filename_vector, "w");

	int32_t rc;

	void *minst;
	rc = gsapi_new_instance(&minst, NULL);

	if (rc < 0)
		return false;

	rc = gsapi_set_arg_encoding(minst, GS_ARG_ENCODING_UTF8);
	if (rc == 0) {
		gsapi_set_stdio(minst, NULL, gsdll_stdout, NULL);
		rc = gsapi_init_with_args(minst, gs_argc, gs_argv);
	}

    int32_t rc2 = gsapi_exit(minst);
    if ((rc == 0) || (rc2 == gs_error_Quit))
		rc = rc2;

	fclose(fn_vector);

	gsapi_delete_instance(minst);

	if (rc)
		return false;

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
static bool ps_to_eps(FILE *ps_file, FILE *eps_file)
{
	int xoffset = 0;
	int yoffset = 0;

	int l;
	while (fgets((char *)buf, sizeof (buf), ps_file)) {
		fprintf(eps_file, "%s", (char *)buf);
		if (*buf != '%')
			break;

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

				if (xoffset || yoffset)
					fprintf(eps_file, "%d %d translate\n", -xoffset, -yoffset);

				if (flip)
					fprintf(eps_file, "%d 0 translate -1 1 scale\n", width);
			}
		}

		if (!strncasecmp((char *) buf, "%!", 2)) {
			fprintf
				(eps_file,
				 "/=== {(        ) cvs print} def" // print a number
				 "/stroke {"
				 // check for solid red
				 "currentrgbcolor "
				 "0.0 eq "
				 "exch 0.0 eq "
				 "and "
				 "exch 1.0 eq "
				 "and "
				 // check for solid blue
				 "currentrgbcolor "
				 "0.0 eq "
				 "exch 1.0 eq "
				 "and "
				 "exch 0.0 eq "
				 "and "
				 "or "
				 // check for solid blue
				 "currentrgbcolor "
				 "1.0 eq "
				 "exch 0.0 eq "
				 "and "
				 "exch 0.0 eq "
				 "and "
				 "or "
				 "{"
				 // solid red, green or blue
				 "(P)=== "
				 "currentrgbcolor "
				 "(,)=== "
				 "100 mul round cvi === "
				 "(,)=== "
				 "100 mul round cvi === "
				 "(,)=== "
				 "100 mul round cvi = "
				 "flattenpath "
				 "{ "
				 // moveto
				 "transform (M)=== "
				 "round cvi === "
				 "(,)=== "
				 "round cvi ="
				 "}{"
				 // lineto
				 "transform(L)=== "
				 "round cvi === "
				 "(,)=== "
				 "round cvi ="
				 "}{"
				 // curveto (not implemented)
				 "}{"
				 // closepath
				 "(C)="
				 "}"
				 "pathforall newpath"
				 "}"
				 "{"
				 // Default is to just stroke
				 "stroke"
				 "}"
				 "ifelse"
				 "}bind def"
				 "/showpage {(X)= showpage}bind def"
				 "\n");

			if (print_raster_mode != 'c' && print_raster_mode != 'g') {
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
	while ((l = fread ((char *) buf, 1, sizeof (buf), ps_file)) > 0)
		fwrite ((char *) buf, 1, l, eps_file);

	return true;
}



/**
 * Perform range validation checks on the major global variables to ensure
 * their values are sane. If values are outside accepted tolerances then modify
 * them to be the correct value.
 *
 * @return Nothing
 */
static void range_checks(void)
{
	if (raster_power > 100)
		raster_power = 100;
	else
		if (raster_power < 0)
			raster_power = 0;

	if (raster_speed > 100)
		raster_speed = 100;
	else
		if (raster_speed < 1)
			raster_speed = 1;

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

	for (int i = 0 ; i < VECTOR_PASSES ; i++) {
		if (vector_power[i] > 100)
			vector_power[i] = 100;
		else
			if (vector_power[i] < 0)
				vector_power[i] = 0;

		if (vector_speed[i] > 100)
			vector_speed[i] = 100;
		else
			if (vector_speed[i] < 1)
				vector_speed[i] = 1;
	}
}

static void usage(int rc, const char * const msg)
{
	static const char usage_str[] =
		"Usage: " PACKAGE " [OPTION]... [FILE]\n"
		"\n"
		"General options:\n"
		"    -a, --autofocus                   Enable auto focus\n"
		"    -n JOBNAME, --job=JOBNAME         Set the job name to display\n"
		"    -p ADDRESS, --printer=ADDRESS     ADDRESS of the printer\n"
		"    -P PRESET, --preset=PRESET        Select a default preset\n"
		"\n"
		"Raster options:\n"
		"    -d DPI, --dpi DPI                 Resolution of raster artwork\n"
		"    -m MODE, --mode MODE              Mode for rasterization (default mono)\n"
		"    -r SPEED, --raster-speed=SPEED    Raster speed\n"
		"    -R POWER, --raster-power=POWER    Raster power\n"
		"    -s SIZE, --screen-size SIZE       Photograph screen size (default 8)\n"
		"\n"
		"Vector options:\n"
		"    -O, --no-optimize                 Disable vector optimization\n"
		"    -f FREQ, --frequency=FREQ         Vector frequency\n"
		"    -v SPEED, --vector-speed=SPEED    Vector speed\n"
		"    -V POWER, --vector-power=POWER    Vector power for the R,G and B passes\n"
		"\n"
		"General options:\n"
		"    -D, --debug                       Enable debug mode\n"
		"    -h, --help                        Output a usage message and exit\n"
		"    --version                         Output the version number and exit\n"
		"";

	fprintf(stderr, "%s%s\n", msg, usage_str);

	exit(rc);
}

static const struct option long_options[] = {
	{ "debug",         no_argument,        NULL,  'D' },
	{ "printer",       required_argument,  NULL,  'p' },
	{ "preset",        required_argument,  NULL,  'P' },
	{ "autofocus",     required_argument,  NULL,  'a' },
	{ "job",           required_argument,  NULL,  'n' },
	{ "dpi",           required_argument,  NULL,  'd' },
	{ "raster-power",  required_argument,  NULL,  'R' },
	{ "raster-speed",  required_argument,  NULL,  'r' },
	{ "mode",          required_argument,  NULL,  'm' },
	{ "screen-size",   required_argument,  NULL,  's' },
	{ "frequency",     required_argument,  NULL,  'f' },
	{ "vector-power",  required_argument,  NULL,  'V' },
	{ "vector-speed",  required_argument,  NULL,  'v' },
	{ "no-optimize",   no_argument,        NULL,  'O' },
	{ "help",          no_argument,        NULL,  'h' },
	{ "version",       no_argument,        NULL,  '@' },
	{ NULL,            0,                  NULL,   0  },
};


/*
 * Look for "X,Y,Z" for each power setting, or "X" for all three.
 * Handle the case where we have been given floating point values,
 * even though we only want to deal with integers.
 */
static int vector_param_set(int * const values, const char *arg )
{
	double v[3] = { 0, 0, 0 };
	int rc = sscanf(arg, "%lf,%lf,%lf", &v[0], &v[1], &v[2]);

	if (rc < 1)
		return -1;

	// convert to integer from the floating point representation
	values[0] = v[0];
	values[1] = v[1];
	values[2] = v[2];

	if (rc <= 1)
		values[1] = values[0];

	if (rc <= 2)
		values[2] = values[1];

	return rc;
}


/**
 * Main entry point for the program.
 *
 * @param argc The number of command line options passed to the program.
 * @param argv An array of strings where each string represents a command line
 * argument.
 * @return An integer where 0 represents successful termination, any other
 * value represents an error code.
 */
int main(int argc, char *argv[])
{
	// Create tmp working directory
	char tmpdir_template[1024] = { '\0' };
	snprintf(tmpdir_template, 1024, "%s/%s.XXXXXX", TMP_DIRECTORY, basename(argv[0]));
	char *tmpdir_name = mkdtemp(tmpdir_template);

	if (tmpdir_name == NULL) {
		perror("mkdtemp failed");
		return false;
	}

	const char * host = "192.168.1.4";

	while (true) {
		const char ch = getopt_long(argc,
									argv,
									"Dp:P:n:d:r:R:v:V:g:G:b:B:m:f:s:aO:h",
									long_options,
									NULL
									);
		if (ch <= 0 )
			break;

		switch(ch) {
		case 'D': debug++; break;
		case 'p': host = optarg; break;
		case 'P': usage(EXIT_FAILURE, "Presets are not supported yet\n"); break;
		case 'n': job_name = optarg; break;
		case 'd': resolution = atoi(optarg); break;
		case 'r': raster_speed = atoi(optarg); break;
		case 'R': raster_power = atoi(optarg); break;
		case 'v':
			if (vector_param_set(vector_speed, optarg) < 0)
				usage(EXIT_FAILURE, "unable to parse vector-speed");
			break;
		case 'V':
			if (vector_param_set(vector_power, optarg) < 0)
				usage(EXIT_FAILURE, "unable to parse vector-power");
			break;
		case 'm': print_raster_mode = tolower(*optarg); break;
		case 'f': vector_freq = atoi(optarg); break;
		case 's': screen_size = atoi(optarg); break;
		case 'a': focus = AUTO_FOCUS; break;
		case 'O': do_vector_optimize = 0; break;
		case 'h': usage(EXIT_SUCCESS, ""); break;
		case '@': fprintf(stdout, "%s\n", VERSION); exit(0); break;
		default: usage(EXIT_FAILURE, "Unknown argument\n"); break;
		}
	}

	/* Perform a check over the global values to ensure that they have values
	 * that are within a tolerated range.
	 */
	range_checks();

	// ensure printer host is set
	if (!host)
		usage(EXIT_FAILURE, "Printer host must be specfied\n");

	// If they did not specify a user, get their name
	if (!job_user) {
		uid_t uid = getuid();
		struct passwd * pw = getpwuid(uid);
		job_user = strndup(pw->pw_name, 33);
	}

	// Skip any of the processed arguments
	argc -= optind;
	argv += optind;

	// If there is an argument after, there must be only one
	// and it will be the input postcript / pdf
	if (argc > 1)
		usage(EXIT_FAILURE, "Only one input file may be specified\n");

	const char *source_filename = argc ? strndup(argv[0], FILENAME_NCHARS) : "stdin";

	char *source_basename = strndup(argv[0], FILENAME_NCHARS);
	source_basename = basename(source_basename);

	// If no job name is specified, use just the filename if there

	// are any / in the name.
	if (!job_name) {
		job_name = source_basename;
	}

	job_title = job_name;

	/* Gather the postscript file from either standard input or a filename
	 * specified as a command line argument.
	 */
	FILE * fh_source_cups = argc ? fopen(source_filename, "r") : stdin;
	if (!fh_source_cups) {
		perror(source_filename);
		exit(EXIT_FAILURE);
	}

	// Report the settings on stdout
	printf("Job: %s (%s)\n"
		   "Raster: speed=%d power=%d dpi=%d\n"
		   "Vector: freq=%d speed=%d,%d,%d power=%d,%d,%d\n"
		   "",
		   job_title,
		   job_user,
		   raster_speed,
		   raster_power,
		   resolution,
		   vector_freq,
		   vector_speed[0],
		   vector_speed[1],
		   vector_speed[2],
		   vector_power[0],
		   vector_power[1],
		   vector_power[2]);

	char *target_base = strndup(source_basename, FILENAME_NCHARS);
	char *last_dot = strrchr(target_base, '.');
	if (last_dot != NULL)
		*last_dot = '\0';

	char target_basename[FILENAME_NCHARS] = { '\0' };

	/* Strings designating filenames. */
	//char target_basename[FILENAME_NCHARS] = { '\0' };
	char target_bitmap[FILENAME_NCHARS] = { '\0' };
	char target_eps[FILENAME_NCHARS] = { '\0' };
	char target_pdf[FILENAME_NCHARS] = { '\0' };
	char target_pjl[FILENAME_NCHARS] = { '\0' };
	char target_ps[FILENAME_NCHARS] = { '\0' };
	char target_vector[FILENAME_NCHARS] = { '\0' };

	/* Temporary variables. */
	int l;

	/* Determine and set the names of all files that will be manipulated by the
	 * program.
	 */
	snprintf(target_basename, FILENAME_NCHARS, "%s/%s", tmpdir_name, target_base);
	snprintf(target_bitmap, FILENAME_NCHARS, "%s.bmp", target_basename);
	snprintf(target_eps, FILENAME_NCHARS, "%s.eps", target_basename);
	snprintf(target_pdf, FILENAME_NCHARS, "%s.pdf", target_basename);
	snprintf(target_pjl, FILENAME_NCHARS, "%s.pjl", target_basename);
	snprintf(target_ps, FILENAME_NCHARS, "%s.ps", target_basename);
	snprintf(target_vector, FILENAME_NCHARS, "%s.vector", target_basename);

	/* File handles. */
	FILE *fh_bitmap;
	FILE *fh_pdf;
	FILE *fh_ps;
	FILE *fh_pjl;
	FILE *fh_vector;

	/* Check whether the incoming data is ps or pdf data. */
	fread((char *)buf, 1, 4, fh_source_cups);
	rewind(fh_source_cups);
	if (strncasecmp((char *)buf, "%PDF", 4) == 0) {
		/* We have a pdf file. */

		/* Open the destination pdf file. */
		fh_pdf = fopen(target_pdf, "w");
		if (!fh_pdf) {
			perror(target_pdf);
			return 1;
		}

		/* Write the cups data out to the fh_pdf. */
		while ((l = fread((char *)buf, 1, sizeof(buf), fh_source_cups)) > 0)
			fwrite((char *)buf, 1, l, fh_pdf);

		fclose(fh_source_cups);
		fclose(fh_pdf);

		fprintf(stderr, "Executing pdf2ps\n");
		int gs_argc = 13;
		char *gs_argv[gs_argc];
		gs_argv[0] = "gs";
		gs_argv[1] = "-q";
		gs_argv[2] = "-dNOPAUSE";
		gs_argv[3] = "-dBATCH";
		gs_argv[4] = "-P-";
		gs_argv[5] = "-dSAFER";
		gs_argv[6] = "-sDEVICE=ps2write";

		gs_argv[7] = calloc(1024, sizeof(char));
		snprintf(gs_argv[7], 1024, "-sOutputFile=%s.ps", target_basename);

		gs_argv[8] = "-c";
		gs_argv[9] = "save";
		gs_argv[10] = "pop";
		gs_argv[11] = "-f";
		gs_argv[12] = strndup(target_pdf, 1024);

		int32_t rc;
		void *minst;

		rc = gsapi_new_instance(&minst, NULL);
		if (rc < 0)
			return false;

		rc = gsapi_set_arg_encoding(minst, GS_ARG_ENCODING_UTF8);
		if (rc == 0)
			rc = gsapi_init_with_args(minst, gs_argc, gs_argv);

		int32_t rc2 = gsapi_exit(minst);

		if ((rc == 0) || (rc2 == gs_error_Quit))
			rc = rc2;

		gsapi_delete_instance(minst);

		if (rc)
			return false;

		if (!debug) {
			/* Debug is disabled so remove generated pdf file. */
			if (unlink(target_pdf)) {
				perror(target_pdf);
			}
		}

		/* Set fh_ps to the generated ps file. */
		fh_ps = fopen(target_ps, "r");
	} else {
		/* Input file is postscript. Set the fh_ps handle to fh_source_cups. */
		fh_ps = fh_source_cups;
	}

	/* Open the encapsulated postscript file for writing. */
	FILE * const fh_eps = fopen(target_eps, "w");
	if (!fh_eps) {
		perror(target_eps);
		return 1;
	}

	/* Convert postscript to encapsulated postscript. */
	if (!ps_to_eps(fh_ps, fh_eps)) {
		perror("Error converting postscript to encapsulated postscript.");
		fclose(fh_eps);
		return 1;
	}

	/* Cleanup after encapsulated postscript creation. */
	fclose(fh_eps);
	if (fh_ps != stdin) {
		fclose(fh_ps);
		if (unlink(target_ps)) {
			perror(target_ps);
		}
	}

	const char * const raster_string =
		print_raster_mode == 'c' ? "bmp16m" :
		print_raster_mode == 'g' ? "bmpgray" :
		"bmpmono";


	fprintf(stderr, "execute_ghostscript\n");
	if(!execute_ghostscript(target_bitmap,
							target_eps,
							target_vector,
							raster_string,
							resolution )) {
		perror("Failure to execute ghostscript command.\n");
		return 1;
	}

	/* Open file handles needed by generation of the printer job language
	 * file.
	 */
	fh_bitmap = fopen(target_bitmap, "r");
	fh_vector = fopen(target_vector, "r");
	fh_pjl = fopen(target_pjl, "w");
	if (!fh_pjl) {
		perror(target_pjl);
		return 1;
	}

	/* Execute the generation of the printer job language (pjl) file. */
	if (!generate_pjl(fh_bitmap, fh_pjl, fh_vector)) {
		perror("Generation of pjl file failed.\n");
		fclose(fh_pjl);
		return 1;
	}

	/* Close open file handles. */
	fclose(fh_bitmap);
	fclose(fh_pjl);
	fclose(fh_vector);

	/* Cleanup unneeded files provided that debug mode is disabled. */
	if (!debug) {
		if (unlink(target_bitmap)) {
			perror(target_bitmap);
		}
		if (unlink(target_eps)) {
			perror(target_eps);
		}
		if (unlink(target_vector)) {
			perror(target_vector);
		}
	}

	/* Open printer job language file. */
	fh_pjl = fopen(target_pjl, "r");
	if (!fh_pjl) {
		perror(target_pjl);
		return 1;
	}

	/* Send print job to printer. */
	if (!printer_send(host, fh_pjl, job_user, job_title, job_name)) {
		perror("Could not send pjl file to printer.\n");
		return 1;
	}

	fclose(fh_pjl);
	if (!debug) {
		if (unlink(target_pjl)) {
			perror(target_pjl);
		}
	}

	if (!debug) {
		if (rmdir(tmpdir_name) == -1) {
			perror("rmdir failed: ");
			return 1;
		}
	}

	return 0;
}
