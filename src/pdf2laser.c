
/// pdf2laser.c --- tool for printing to Epilog Fusion laser cutters

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


FILE *fh_vector;
static int GSDLLCALL gsdll_stdout(void *minst, const char *str, int len)
{
	size_t rc = fwrite(str, 1, len, fh_vector);
	fflush(fh_vector);
	return rc;
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
static bool execute_ghostscript(print_job_t *print_job,
								const char * const filename_bitmap,
								const char * const filename_eps,
								const char * const filename_vector,
								const char * const bmp_mode)
{

	int gs_argc = 8;
	char *gs_argv[8];

	gs_argv[0] = "gs";
	gs_argv[1] = "-q";
	gs_argv[2] = "-dBATCH";
	gs_argv[3] = "-dNOPAUSE";

	gs_argv[4] = calloc(64, sizeof(char));
	snprintf(gs_argv[4], 64, "-r%d", print_job->raster->resolution);

	gs_argv[5] = calloc(1024, sizeof(char));
	snprintf(gs_argv[5], 1024, "-sDEVICE=%s", bmp_mode);

	gs_argv[6] = calloc(1024, sizeof(char));
	snprintf(gs_argv[6], 1024, "-sOutputFile=%s", filename_bitmap);

	gs_argv[7] = calloc(1024, sizeof(char));
	snprintf(gs_argv[7], 1024, "%s", filename_eps);

	fh_vector = fopen(filename_vector, "w");

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

	fclose(fh_vector);

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
static bool ps_to_eps(print_job_t *print_job, FILE *ps_file, FILE *eps_file)
{

	char *line = NULL;
	size_t length = 0;
	ssize_t length_read;

	while ((length_read = getline(&line, &length, ps_file)) != -1) {
		fprintf(eps_file, "%s", line);

		if (*line != '%') {
			break;
		}
		else if (strncmp(line, "%!", 2) == 0) {
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

			if (print_job->raster->mode != 'c' && print_job->raster->mode != 'g') {
				if (print_job->raster->screen_size == 0) {
					fprintf(eps_file, "{0.5 ge{1}{0}ifelse}settransfer\n");
				}
				else {
					uint32_t screen_size = print_job->raster->screen_size;
					if (print_job->raster->resolution >= 600) {
						// adjust for overprint
						fprintf(eps_file,
								"{dup 0 ne{%d %d div add}if}settransfer\n",
								print_job->raster->resolution / 600, screen_size);
					}
					fprintf(eps_file, "%d 30{%s}setscreen\n", print_job->raster->resolution / screen_size,
							(print_job->raster->screen_size > 0) ? "pop abs 1 exch sub" :
							"180 mul cos exch 180 mul cos add 2 div");
				}
			}
		}
		else if (strncasecmp(line, "%%PageBoundingBox:", 18) == 0) {

			int32_t x_offset = 0;
			int32_t y_offset = 0;

			int32_t x_lower_left, y_lower_left, x_upper_right, y_upper_right;

			if (sscanf(line, "%%%%PageBoundingBox: %d %d %d %d",
					   &x_lower_left,
					   &y_lower_left,
					   &x_upper_right,
					   &y_upper_right) == 4) {

				x_offset = x_lower_left;
				y_offset = y_lower_left;

				print_job->width = (x_upper_right - x_lower_left);
				print_job->height = (y_upper_right - y_lower_left);

				fprintf(eps_file, "/setpagedevice{pop}def\n"); // use bbox

				if (x_offset || y_offset)
					fprintf(eps_file, "%d %d translate\n", -x_offset, -y_offset);

				if (print_job->flip)
					fprintf(eps_file, "%d 0 translate -1 1 scale\n", print_job->width);
			}
		}
	}

	free(line);

	{
		size_t length;
		uint8_t buffer[102400];
		while ((length = fread(buffer, 1, 102400, ps_file)) > 0)
			fwrite(buffer, 1, length, eps_file);
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
static void range_checks(print_job_t *print_job)
{
	if (print_job->raster->power > 100) {
		print_job->raster->power = 100;
	}
	else if (print_job->raster->power < 0) {
		print_job->raster->power = 0;
	}

	if (print_job->raster->speed > 100) {
		print_job->raster->speed = 100;
	}
	else if (print_job->raster->speed < 1) {
		print_job->raster->speed = 1;
	}

	if (print_job->raster->resolution > 1200) {
		print_job->raster->resolution = 1200;
	}
	else if (print_job->raster->resolution < 75) {
		print_job->raster->resolution = 75;
	}

	if (print_job->raster->screen_size < 1) {
		print_job->raster->screen_size = 1;
	}

	if (print_job->vector_frequency < 10) {
		print_job->vector_frequency = 10;
	}
	else if (print_job->vector_frequency > 5000) {
		print_job->vector_frequency = 5000;
	}

	for (int i = 0 ; i < VECTOR_PASSES ; i++) {
		if (print_job->vectors[i]->power > 100) {
			print_job->vectors[i]->power = 100;
		}
		else if (print_job->vectors[i]->power < 0) {
			print_job->vectors[i]->power = 0;
		}

		if (print_job->vectors[i]->speed > 100) {
			print_job->vectors[i]->speed = 100;
		}
		else if (print_job->vectors[i]->speed < 1) {
			print_job->vectors[i]->speed = 1;
		}
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


static int32_t vector_set_param_speed(print_job_t *print_job, char *optarg)
{
	double values[3] = { 0.0, 0.0, 0.0 };
	int32_t rc = sscanf(optarg, "%lf,%lf,%lf", &values[0], &values[1], &values[2]);

	if (rc < 1)
		return -1;

	print_job->vectors[0]->speed = (int32_t)values[0];
	print_job->vectors[1]->speed = (int32_t)values[1];
	print_job->vectors[2]->speed = (int32_t)values[2];

	if (rc <= 1)
		print_job->vectors[1]->speed = print_job->vectors[0]->speed;

	if (rc <= 2)
		print_job->vectors[2]->speed = print_job->vectors[1]->speed;

	return rc;
}

static int32_t vector_set_param_power(print_job_t *print_job, char *optarg)
{
	double values[3] = { 0.0, 0.0, 0.0 };
	int32_t rc = sscanf(optarg, "%lf,%lf,%lf", &values[0], &values[1], &values[2]);

	if (rc < 1)
		return -1;

	print_job->vectors[0]->power = (int32_t)values[0];
	print_job->vectors[1]->power = (int32_t)values[1];
	print_job->vectors[2]->power = (int32_t)values[2];

	if (rc <= 1)
		print_job->vectors[1]->power = print_job->vectors[0]->power;

	if (rc <= 2)
		print_job->vectors[2]->power = print_job->vectors[1]->power;

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

	print_job_t *print_job = &(print_job_t){
		.flip = FLIP,
		.height = BED_HEIGHT,
		.width = BED_WIDTH,
		.focus = false,
		.raster = &(raster_t){
			.resolution = RESOLUTION_DEFAULT,
			.mode = RASTER_MODE_DEFAULT,
			.speed = RASTER_SPEED_DEFAULT,
			.power = RASTER_POWER_DEFAULT,
			.repeat = RASTER_REPEAT,
			.screen_size = SCREEN_DEFAULT,
		},
		.vector_frequency = VECTOR_FREQUENCY_DEFAULT,
		.vector_optimize = true,
		.vectors = NULL,
		.debug = DEBUG,
	};

	print_job->vectors = calloc(VECTOR_PASSES, sizeof(vector_list_t*));
	for (int32_t i = 0; i < VECTOR_PASSES; i++) {
		print_job->vectors[i] = vector_list_create();
	}

	const char *host = "192.168.1.4";

	while (true) {
		const char ch = getopt_long(argc,
									argv,
									"Dp:P:n:d:r:R:v:V:g:G:b:B:m:f:s:aO:h",
									long_options,
									NULL
									);
		if (ch <= 0 )
			break;

		switch (ch) {
		case 'D': print_job->debug = true; break;
		case 'p': host = optarg; break;
		case 'P': usage(EXIT_FAILURE, "Presets are not supported yet\n"); break;
		case 'n': print_job->name = optarg; break;
		case 'd': print_job->raster->resolution = atoi(optarg); break;
		case 'r': print_job->raster->speed = atoi(optarg); break;
		case 'R': print_job->raster->power = atoi(optarg); break;
		case 'v':
			if (vector_set_param_speed(print_job, optarg) < 0)
				usage(EXIT_FAILURE, "unable to parse vector-speed");
			break;
		case 'V':
			if (vector_set_param_power(print_job, optarg) < 0)
				usage(EXIT_FAILURE, "unable to parse vector-power");
			break;
		case 'm': print_job->raster->mode = tolower(*optarg); break;
		case 'f': print_job->vector_frequency = atoi(optarg); break;
		case 's': print_job->raster->screen_size = atoi(optarg); break;
		case 'a': print_job->focus = AUTO_FOCUS; break;
		case 'O': print_job->vector_optimize = false; break;
		case 'h': usage(EXIT_SUCCESS, ""); break;
		case '@': fprintf(stdout, "%s\n", VERSION); exit(0); break;
		default: usage(EXIT_FAILURE, "Unknown argument\n"); break;
		}
	}

	/* Perform a check over the global values to ensure that they have values
	 * that are within a tolerated range.
	 */
	range_checks(print_job);

	// ensure printer host is set
	if (!host)
		usage(EXIT_FAILURE, "Printer host must be specfied\n");

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
	if (!print_job->name) {
		print_job->name = source_basename;
	}

	// Report the settings on stdout
	printf("Job: %s\n"
		   "Raster: speed=%d power=%d dpi=%d\n"
		   "Vector: freq=%d speed=%d,%d,%d power=%d,%d,%d\n"
		   "",
		   print_job->name,
		   print_job->raster->speed,
		   print_job->raster->power,
		   print_job->raster->resolution,
		   print_job->vector_frequency,
		   print_job->vectors[0]->speed,
		   print_job->vectors[1]->speed,
		   print_job->vectors[2]->speed,
		   print_job->vectors[0]->power,
		   print_job->vectors[1]->power,
		   print_job->vectors[2]->power);

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

	if (strncmp(source_filename, "stdin", 5) == 0) {
		fh_pdf = fopen(target_pdf, "w");
		if (!fh_pdf) {
			perror(target_pdf);
			return 1;
		}

		{
			uint8_t buffer[102400];
			size_t rc;
			while ((rc = fread(buffer, 1, 102400, stdin)) > 0)
				fwrite(buffer, 1, rc, fh_pdf);
		}

		fclose(fh_pdf);
	}

	if (!generate_ps(source_filename, target_ps)) {
		perror("Error converting pdf to postscript.");
		return 1;
	}

	if (!print_job->debug) {
		/* Debug is disabled so remove generated pdf file. */
		if (unlink(target_pdf)) {
			perror(target_pdf);
		}
	}

	fh_ps = fopen(target_ps, "r");
	if (!fh_ps) {
		perror("Error opening postscript file.");
		return 1;
	}

	/* Open the encapsulated postscript file for writing. */
	FILE * const fh_eps = fopen(target_eps, "w");
	if (!fh_eps) {
		perror(target_eps);
		return 1;
	}

	/* Convert postscript to encapsulated postscript. */
	if (!ps_to_eps(print_job, fh_ps, fh_eps)) {
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
		print_job->raster->mode == 'c' ? "bmp16m" :
		print_job->raster->mode == 'g' ? "bmpgray" :
		"bmpmono";


	fprintf(stderr, "execute_ghostscript\n");
	if(!execute_ghostscript(print_job,
							target_bitmap,
							target_eps,
							target_vector,
							raster_string)) {
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
	if (!generate_pjl(print_job, fh_bitmap, fh_pjl, fh_vector)) {
		perror("Generation of pjl file failed.\n");
		fclose(fh_pjl);
		return 1;
	}

	/* Close open file handles. */
	fclose(fh_bitmap);
	fclose(fh_pjl);
	fclose(fh_vector);

	/* Cleanup unneeded files provided that debug mode is disabled. */
	if (!print_job->debug) {
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
	if (!printer_send(host, fh_pjl, print_job->name)) {
		perror("Could not send pjl file to printer.\n");
		return 1;
	}

	fclose(fh_pjl);
	if (!print_job->debug) {
		if (unlink(target_pjl)) {
			perror(target_pjl);
		}
	}

	if (!print_job->debug) {
		if (rmdir(tmpdir_name) == -1) {
			perror("rmdir failed: ");
			return 1;
		}
	}

	return 0;
}
