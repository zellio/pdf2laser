#include "pdf2laser_cli.h"
#include <ctype.h>                    // for tolower
#include <stddef.h>                   // for NULL, offsetof, size_t
#include <stdint.h>                   // for int32_t, uint64_t, uint8_t
#include <stdio.h>                    // for fprintf, sscanf, stderr, stdout
#include <stdlib.h>                   // for atoi, exit, EXIT_FAILURE, calloc, free, EXIT_SUCCESS
#include <string.h>                   // for strndup, strtok, strncmp, strncpy, strnlen
#include "config.h"                   // for FILENAME_NCHARS, HOSTNAME_NCHARS, PACKAGE, VERSION
#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "optparse.h"                 // for OPTPARSE_REQUIRED, optparse, OPTPARSE_NONE, optparse_long, optparse_init
#include "type_preset.h"              // for preset_apply_to_print_job, preset_t
#include "type_preset_file.h"         // for preset_file_t
#include "type_print_job.h"           // for print_job_t, print_job_append_new_vector_list_config, print_job_find_vector_list_config_by_rgb
#include "type_raster.h"              // for raster_t
#include "type_vector_list_config.h"  // for vector_list_config_t, vector_list_config_id_to_rgb

static const struct optparse_long long_options[] = {
	{"debug",                 'D',  OPTPARSE_NONE},
	{"printer",               'p',  OPTPARSE_REQUIRED},
	{"preset",                'P',  OPTPARSE_REQUIRED},
	{"autofocus",             'a',  OPTPARSE_NONE},
	{"job-mode",              'j',  OPTPARSE_REQUIRED},
	{"job",                   'n',  OPTPARSE_REQUIRED},
	{"raster-power",          'R',  OPTPARSE_REQUIRED},
	{"raster-speed",          'r',  OPTPARSE_REQUIRED},
	{"raster-dpi",            'd',  OPTPARSE_REQUIRED},
	{"raster-mode",           'm',  OPTPARSE_REQUIRED},
	{"screen-size",           's',  OPTPARSE_REQUIRED},
	{"vector-power",          'V',  OPTPARSE_REQUIRED},
	{"vector-speed",          'v',  OPTPARSE_REQUIRED},
	{"vector-frequency",      'f',  OPTPARSE_REQUIRED},
	{"vector-passes",         'M',  OPTPARSE_REQUIRED},
	{"no-vector-optimize",    'O',  OPTPARSE_NONE},
	{"no-vector-fallthrough", 'F',  OPTPARSE_NONE},
	{"help",                  'h',  OPTPARSE_NONE},
	{"version",               '@',  OPTPARSE_NONE},
	{0}
};


static const struct optparse_long preset_long_option[] = {
	{"preset", 'P',  OPTPARSE_REQUIRED},
	{0}
};


static void usage(int rc, const char * const msg)
{
	static const char usage_str[] =
		"Usage: " PACKAGE " [OPTION]... [FILE]\n"
		"\n"
		"General options:\n"
		"  -n, --job=JOBNAME              Set the job name to display\n"
		"  -p, --printer=ADDRESS          ADDRESS of the printer\n"
		"  -j, --job-mode=MODE            Set job mode to Vector, Raster, or Combined\n"
		"  -P, --preset=PRESET            Load configuration preset\n"
		"  -a, --autofocus                Enable auto focus\n"
		"\n"
		"Raster options:\n"
		"  -R, --raster-power=POWER       Laser power for raster pass\n"
		"  -r, --raster-speed=SPEED       Laser head speed for raster pass\n"
		"  -d, --raster-dpi=DPI           Resolution of source file images\n"
		"  -m, --raster-mode=MODE         Mode for rasterization (default mono)\n"
		"  -s, --raster-screen-size=SIZE  Photograph screen size (default 8)\n"
		"\n"
		"Vector options:\n"
		"  -V, --vector-power=POWER       Laser power for vector pass\n"
		"  -v, --vector-speed=SPEED       Laser head speed for vector pass\n"
		"  -f, --vector-frequency=FREQ    Laser frequency for vector pass\n"
		"  -M, --vector-passes=PASSES     Number of times to repeat vector pass\n"
		"  -O, --no-vector-optimize       Disable vector optimization\n"
		"  -F, --no-vector-fallthrough    Disable automatic vector configuration\n"
		"\n"
		"Generic program options:\n"
		"  -D, --debug                    Enable debug mode\n"
		"  -h, --help                     Output a usage message and exit\n"
		"      --version                  Output the version number and exit\n"
		"";

	fprintf(stderr, "%s%s\n", msg, usage_str);

	exit(rc);
}

static int32_t vector_config_set_param_offset(print_job_t *print_job, char *optarg, size_t FIELD)
{
	size_t optlen = strnlen(optarg, OPTARG_MAX_LENGTH);
	char *s = calloc(optlen + 1, sizeof(char));
	char *s_ptr = s;

	strncpy(s, optarg, optlen);

	char *token = strtok(s, ",");
	while (token) {
		uint64_t values[2] = {0, 0};
		int32_t rc = sscanf(token, "%lx=%lu", &values[0], &values[1]);
		if (rc != 2) {
			free(s_ptr);
			return -1;
		}

		int32_t red, green, blue;
		vector_list_config_id_to_rgb(values[0], &red, &green, &blue);

		vector_list_config_t *vector_list_config = print_job_find_vector_list_config_by_rgb(print_job, red, green, blue);
		if (vector_list_config == NULL)
			vector_list_config = print_job_append_new_vector_list_config(print_job, red, green, blue);

		uint8_t *base = (uint8_t *)(vector_list_config);
		int32_t *field = (int32_t *)(base + FIELD);
		*field = (int32_t)values[1];

		token = strtok(NULL, ",");
	}

	free(s_ptr);

	return 0;
}

static int32_t vector_config_set_param_speed(print_job_t *print_job, char *optarg)
{
	return vector_config_set_param_offset(print_job, optarg, offsetof(vector_list_config_t, speed));
}

static int32_t vector_config_set_param_power(print_job_t *print_job, char *optarg)
{
	return vector_config_set_param_offset(print_job, optarg, offsetof(vector_list_config_t, power));
}

static int32_t vector_config_set_param_multipass(print_job_t *print_job, char *optarg)
{
	return vector_config_set_param_offset(print_job, optarg, offsetof(vector_list_config_t, multipass));
}

static int32_t vector_config_set_param_frequency(print_job_t *print_job, char *optarg)
{
	return vector_config_set_param_offset(print_job, optarg, offsetof(vector_list_config_t, frequency));
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

	for (vector_list_config_t *current_config = print_job->configs; current_config != NULL; current_config = current_config->next) {
		if (current_config->power > 100) {
			current_config->power = 100;
		}
		else if (current_config->power < 0) {
			current_config->power = 0;
		}

		if (current_config->speed > 100) {
			current_config->speed = 100;
		}
		else if (current_config->speed < 1) {
			current_config->speed = 1;
		}

		if (current_config->multipass < 1) {
			current_config->multipass = 1;
		}

		if (current_config->frequency < 10) {
			current_config->frequency = 10;
		}
		else if (current_config->frequency > 5000) {
			current_config->frequency = 5000;
		}
	}
}

bool pdf2laser_optparse(print_job_t *print_job, preset_file_t **preset_files, size_t preset_files_count, int32_t argc, char **argv)
{
	struct optparse options;
	int option;

	// First we look for a preset file because command line options override preset values.
	optparse_init(&options, argv);

	char *preset = NULL;
	while ((option = optparse_long(&options, preset_long_option, NULL)) != -1) {
		switch (option) {
		case 'P':
			preset = strndup(options.optarg, FILENAME_NCHARS);
			break;
		default:
			continue;
		}
	}

	if (preset) {
		uint8_t preset_found = 0;
		for (size_t index = 0; index < preset_files_count; index += 1) {
			if (strncmp(preset, preset_files[index]->preset->name, FILENAME_NCHARS)) {
				continue;
			}
			preset_apply_to_print_job(preset_files[index]->preset, print_job);
			preset_found = 1;
			break;
		}

		if (preset_found == 0) {
			fprintf(stderr, "Invalid preset: %s\nValid presets:", preset);
			for (size_t index = 0; index < preset_files_count; index += 1) {
				fprintf(stderr, "\n - %s", preset_files[index]->preset->name);
			}
			exit(-1);
		}

		free(preset);
	}

	// Now we load command line options
	optparse_init(&options, argv);

	while ((option = optparse_long(&options, long_options, NULL)) != -1) {
		switch (option) {
		case 'D':
			print_job->debug = true;
			break;

		case 'p':
			print_job->host = strndup(options.optarg, HOSTNAME_NCHARS);
			break;

		case 'P':
			// handled above
			break;

		case 'n':
			print_job->name = strndup(options.optarg, FILENAME_NCHARS);
			break;

		case 'j':
			print_job->mode = tolower(*options.optarg);
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
			if (vector_config_set_param_speed(print_job, options.optarg) < 0)
				usage(EXIT_FAILURE, "unable to parse vector-speed");
			break;

		case 'V':
			if (vector_config_set_param_power(print_job, options.optarg) < 0)
				usage(EXIT_FAILURE, "unable to parse vector-power");
			break;

		case 'M':
			if (vector_config_set_param_multipass(print_job, options.optarg) < 0)
				usage(EXIT_FAILURE, "unable to parse multipass");
			break;

		case 'm':
			print_job->raster->mode = tolower(*options.optarg);
			break;

		case 'f':
			if (vector_config_set_param_frequency(print_job, options.optarg) < 0)
				usage(EXIT_FAILURE, "unable to parse frequency");
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

		case 'F':
			print_job->vector_fallthrough = false;
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

	print_job->source_filename = argc ? strndup(argv[0], FILENAME_NCHARS) : "stdin";

	return true;
}
