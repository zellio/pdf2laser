#include "pdf2laser_cli.h"

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "optparse.h"

#include <ctype.h>                  // for tolower
//#include <getopt.h>                 // for optarg, required_argument, no_arg...
#include <stdint.h>                 // for int32_t
#include <stdio.h>                  // for NULL, fprintf, sscanf, stderr
#include <stdlib.h>                 // for atoi, EXIT_FAILURE, exit, EXIT_SU...
#include <string.h>                 // for strndup
#include "config.h"                 // for PACKAGE, VERSION
#include "pdf2laser_type.h"         // for print_job_t, raster_t
#include "pdf2laser_vector_list.h"  // for vector_list_t

static const struct optparse_long long_options[] = {
	{"debug",        'D', OPTPARSE_NONE},
	{"printer",      'p', OPTPARSE_REQUIRED},
	{"preset",       'P', OPTPARSE_REQUIRED},
	{"autofocus",    'a', OPTPARSE_NONE},
	{"job",          'n', OPTPARSE_REQUIRED},
	{"dpi",          'd', OPTPARSE_REQUIRED},
	{"raster-power", 'R', OPTPARSE_REQUIRED},
	{"raster-speed", 'r', OPTPARSE_REQUIRED},
	{"mode",         'm', OPTPARSE_REQUIRED},
	{"screen-size",  's', OPTPARSE_REQUIRED},
	{"frequency",    'f', OPTPARSE_REQUIRED},
	{"vector-power", 'V', OPTPARSE_REQUIRED},
	{"vector-speed", 'v', OPTPARSE_REQUIRED},
	{"multipass",    'M', OPTPARSE_REQUIRED},
	{"no-optimize",  'O', OPTPARSE_NONE},
	{"help",         'h', OPTPARSE_NONE},
	{"version",      '@', OPTPARSE_NONE},
	{0}
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
		"    -V POWER, --vector-power=POWER    Vector power for the R, G, and B passes\n"
	        "    -M PASSES, --multipass=PASSES     Number of times to repeat the R, G, and B passes\n"
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


static int32_t vector_set_param_multipass(print_job_t *print_job, char *optarg)
{
	int32_t values[3] = { 0, 0, 0 };
	int32_t rc = sscanf(optarg, "%d,%d,%d", &values[0], &values[1], &values[2]);

	if (rc < 1)
		return -1;

	print_job->vectors[0]->multipass = values[0];
	print_job->vectors[1]->multipass = values[1];
	print_job->vectors[2]->multipass = values[2];

	if (rc <= 1)
		print_job->vectors[1]->multipass = print_job->vectors[0]->multipass;

	if (rc <= 2)
		print_job->vectors[2]->multipass = print_job->vectors[1]->multipass;

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

		if (print_job->vectors[i]->multipass < 1) {
			print_job->vectors[i]->multipass = 1;
		}
	}
}

bool pdf2laser_optparse(print_job_t *print_job, int32_t argc, char **argv)
{
	struct optparse options;
	int option;

	optparse_init(&options, argv);

	while ((option = optparse_long(&options, long_options, NULL)) != -1) {
		switch (option) {
		case 'D':
			print_job->debug = true;
			break;

		case 'p':
			print_job->host = options.optarg;
			break;

		case 'P':
			usage(EXIT_FAILURE, "Presets are not supported yet\n");
			break;

		case 'n':
			print_job->name = options.optarg;
			break;

		case 'd':
			print_job->raster->resolution = atoi(options.optarg);
			break;

		case 'r':
			print_job->raster->speed = atoi(options.optarg);
			break;

		case 'R':
			print_job->raster->power = atoi(options.optarg);
			break;

		case 'v':
			if (vector_set_param_speed(print_job, options.optarg) < 0)
				usage(EXIT_FAILURE, "unable to parse vector-speed");
			break;

		case 'V':
			if (vector_set_param_power(print_job, options.optarg) < 0)
				usage(EXIT_FAILURE, "unable to parse vector-power");
			break;

		case 'M':
			if (vector_set_param_multipass(print_job, options.optarg) < 0)
				usage(EXIT_FAILURE, "unable to parse multipass");
			break;

		case 'm':
			print_job->raster->mode = tolower(*options.optarg);
			break;

		case 'f':
			print_job->vector_frequency = atoi(options.optarg);
			break;

		case 's':
			print_job->raster->screen_size = atoi(options.optarg);
			break;

		case 'a':
			print_job->focus = true;
			break;

		case 'O':
			print_job->vector_optimize = false;
			break;

		case 'h':
			usage(EXIT_SUCCESS, "");
			break;

		case '@':
			fprintf(stdout, "%s\n", VERSION);
			exit(EXIT_SUCCESS);

		case '?':
			fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
			exit(EXIT_FAILURE);

		default:
			usage(EXIT_FAILURE, "Unknown argument\n");

		}
	}

	range_checks(print_job);

	// Skip any of the processed arguments
	argc -= options.optind;
	argv += options.optind;

	// If there is an argument after, there must be only one
	// and it will be the input postcript / pdf
	if (argc > 1)
		usage(EXIT_FAILURE, "Only one input file may be specified\n");

	print_job->source_filename = argc ? strndup(argv[0], 1024) : "stdin";

	return true;
}
