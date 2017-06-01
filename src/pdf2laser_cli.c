#include "pdf2laser_cli.h"
#include <ctype.h>                  // for tolower
#include <getopt.h>                 // for optarg, required_argument, no_arg...
#include <stdint.h>                 // for int32_t
#include <stdio.h>                  // for NULL, fprintf, sscanf, stderr
#include <stdlib.h>                 // for atoi, EXIT_FAILURE, exit, EXIT_SU...
#include <string.h>                 // for strndup
#include "config.h"                 // for PACKAGE, VERSION
#include "pdf2laser_type.h"         // for print_job_t, raster_t
#include "pdf2laser_vector_list.h"  // for vector_list_t

static const char *opt_string = "Dp:P:n:d:r:R:v:V:g:G:b:B:m:f:s:aO:h";
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

	for (int i = 0 ; i < 3 ; i++) {
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

bool optparse(print_job_t *print_job, int32_t argc, char **argv)
{
	int32_t long_index = 0;

	int32_t opt;
	while ((opt = getopt_long(argc, argv, opt_string, long_options, &long_index)) != -1) {
		switch (opt) {
		case 'D': print_job->debug = true; break;
		case 'p': print_job->host = optarg; break;
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
		case 'a': print_job->focus = true; break;
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

	// Skip any of the processed arguments
	argc -= optind;
	argv += optind;

	// If there is an argument after, there must be only one
	// and it will be the input postcript / pdf
	if (argc > 1)
		usage(EXIT_FAILURE, "Only one input file may be specified\n");

	print_job->source_filename = argc ? strndup(argv[0], 1024) : "stdin";

	return true;
}
