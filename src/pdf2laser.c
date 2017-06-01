
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
#include <errno.h>                  // for errno, EAGAIN, EINTR
#include <ghostscript/gserrors.h>   // for gs_error_type::gs_error_Quit
#include <ghostscript/iapi.h>       // for gsapi_new_instance, gsapi_set_arg_encoding, gsapi_set_stdio, gsapi_init_with_args, gsapi_delete_instance, gsapi_exit
#include <libgen.h>                 // for basename
#include <stdint.h>                 // for int32_t, uint8_t
#include <stdio.h>                  // for perror, snprintf, fclose, fopen
#include <stdlib.h>                 // for calloc, mkdtemp
#include <string.h>                 // for strrchr, strncmp
#ifdef __linux
#include <sys/sendfile.h>           // for sendfile
#endif
#include <sys/stat.h>               // for fstat, stat
#include <unistd.h>                 // for unlink, rmdir, ssize_t
#include "pdf2laser_cli.h"          // for optparse
#include "pdf2laser_generator.h"    // for VECTOR_PASSES, generate_ps, generate_eps, generate_pjl
#include "pdf2laser_printer.h"      // for pritner_send
#include "pdf2laser_type.h"         // for print_job_t, raster_t
#include "pdf2laser_vector_list.h"  // for vector_list_t, vector_list_create
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
	// This is the NYC Resistor laser host.
	char *host = "192.168.1.4";

	// Job struct defaults
	print_job_t *print_job = &(print_job_t){
		// .flip = FLIP,
		.host = host,
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

	// NOTE: This should be replaced with something that processes the colours
	// that are passed in so pdf2laser can support something other than the
	// current RGB layout.
	print_job->vectors = calloc(VECTOR_PASSES, sizeof(vector_list_t*));
	for (int32_t i = 0; i < VECTOR_PASSES; i++) {
		print_job->vectors[i] = vector_list_create();
	}

	// Process command line options
	optparse(print_job, argc, argv);

	const char *source_filename = print_job->source_filename;

	char *source_basename = strndup(print_job->source_filename, FILENAME_NCHARS);
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

	fh_pdf = fopen(target_pdf, "w");
	if (!fh_pdf) {
		perror(target_pdf);
		return 1;
	}

	if (strncmp(source_filename, "stdin", 5) == 0) {
		uint8_t buffer[102400];
		size_t rc;
		while ((rc = fread(buffer, 1, 102400, stdin)) > 0)
			fwrite(buffer, 1, rc, fh_pdf);
	}
	else {
		FILE *fh_source = fopen(source_filename, "r");
		int32_t source_fno = fileno(fh_source);

		struct stat file_stat;
		if (fstat(source_fno, &file_stat)) {
			perror("Error reading pjl file\n");
			return false;
		}

#ifdef __linux
		ssize_t bs = 0;
		size_t bytes_sent = 0;
		size_t count = file_stat.st_size;

		while (bytes_sent < count) {
			if ((bs = sendfile(fileno(fh_pdf), source_fno, 0, count - bytes_sent)) <= 0) {
				if (errno == EINTR || errno == EAGAIN)
					continue;
				perror("sendfile filed");
				return -1;
			}
			bytes_sent += bs;
		}
#else
		{
			char buffer[102400];
			size_t rc;
			while ((rc = fread(buffer, 1, 102400, fh_source)) > 0)
				fwrite(buffer, 1, rc, fh_pdf);
		}

#endif

		fclose(fh_source);
	}

	fclose(fh_pdf);

	if (!generate_ps(target_pdf, target_ps)) {
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
	if (!generate_eps(print_job, fh_ps, fh_eps)) {
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
	if (!printer_send(print_job->host, fh_pjl, print_job->name)) {
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
