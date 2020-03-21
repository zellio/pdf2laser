/// pdf2laser.c --- tool for printing to Epilog Fusion laser cutters

// Copyright (C) 2015-2020 Zachary Elliott <contact@zell.io>
// Copyright (C) 2011-2015 Trammell Hudson <hudson@osresearch.net>
// Copyright (C) 2008-2011 AS220 Labs <brandon@as220.org>
// Copyright (C) 2002-2008 Andrews & Arnold Ltd <info@aaisp.net.uk>

// Authors: Andrew & Arnold Ltd <info@aaisp.net.uk>
//          Brandon Edens <brandon@as220.org>
//          Trammell Hudson <hudson@osresearch.net>
//          Zachary Elliott <contact@zell.io>
// URL: https://github.com/zellio/pdf2laser
// Version: 0.5.1

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

#include "pdf2laser.h"
#include <dirent.h>                // for closedir, opendir, readdir, DIR, dirent
#include <ghostscript/gserrors.h>  // for gs_error_Quit
#include <ghostscript/iapi.h>      // for gsapi_delete_instance, gsapi_exit, gsapi_init_with_args, gsapi_new_instance, gsapi_set_arg_encoding, gsapi_set_stdio, GSDLLCALL, GS_ARG_ENCODING_UTF8
#include <libgen.h>                // for basename
#include <limits.h>                // for PATH_MAX
#include <stdbool.h>               // for false
#include <stddef.h>                // for size_t, NULL
#include <stdint.h>                // for int32_t
#include <stdio.h>                 // for perror, snprintf, fclose, fflush, fopen, fwrite, printf, FILE
#include <stdlib.h>                // for free, calloc, getenv, mkdtemp
#include <string.h>                // for strndup, strnlen, strrchr
#include <sys/stat.h>              // for stat, S_ISREG
#include <unistd.h>                // for unlink, rmdir
#include "config.h"                // for FILENAME_NCHARS, DEBUG, TMP_DIRECTORY
#include "pdf2laser_cli.h"         // for pdf2laser_optparse
#include "pdf2laser_generator.h"   // for generate_eps, generate_pdf, generate_pjl, generate_ps
#include "pdf2laser_printer.h"     // for printer_send
#include "pdf2laser_util.h"        // for pdf2laser_format_string
#include "type_preset_file.h"      // for preset_file_t, preset_file_create, preset_file_destroy
#include "type_print_job.h"        // for print_job_t, print_job_create, print_job_destroy, print_job_to_string
#include "type_raster.h"           // for raster_t

FILE *fh_vector;
static int GSDLLCALL gsdll_stdout(__attribute__ ((unused)) void *minst, const char *str, int len)
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
static int execute_ghostscript(print_job_t *print_job, const char *const target_eps, const char *const target_bmp, const char *const target_vector, const char *const raster_string)
{
	int gs_argc = 8;
	char *gs_argv[8];

	gs_argv[0] = "gs";
	gs_argv[1] = "-q";
	gs_argv[2] = "-dBATCH";
	gs_argv[3] = "-dNOPAUSE";
	gs_argv[4] = pdf2laser_format_string("-r%d", print_job->raster->resolution);
	gs_argv[5] = pdf2laser_format_string("-sDEVICE=%s", raster_string);
	gs_argv[6] = pdf2laser_format_string("-sOutputFile=%s", target_bmp);
	gs_argv[7] = strndup(target_eps, FILENAME_NCHARS);

	fh_vector = fopen(target_vector, "w");

	int32_t rc;

	void *minst = NULL;
	rc = gsapi_new_instance(&minst, NULL);

	if (rc < 0)
		goto terminate_execute_ghostscript;

	rc = gsapi_set_arg_encoding(minst, GS_ARG_ENCODING_UTF8);
	if (rc == 0) {
		gsapi_set_stdio(minst, NULL, gsdll_stdout, NULL);
		rc = gsapi_init_with_args(minst, gs_argc, gs_argv);
	}

	int32_t rc2 = gsapi_exit(minst);
	if ((rc == 0) || (rc2 == gs_error_Quit))
		rc = rc2;

	gsapi_delete_instance(minst);

 terminate_execute_ghostscript:
	fclose(fh_vector);

	free(gs_argv[4]);
	free(gs_argv[5]);
	free(gs_argv[6]);
	free(gs_argv[7]);

	return rc;
}

static char *append_directory(char *base_directory, char *directory_name)
{
	static const char *path_template = "%s/%s";

	size_t path_length = 1;
	path_length += snprintf(NULL, 0, path_template, base_directory, directory_name);

	char *s = calloc(path_length, sizeof(char));
	snprintf(s, path_length, path_template, base_directory, directory_name);

	return s;
}

static int pdf2laser_load_presets(preset_file_t ***preset_files, size_t *preset_files_count) {
	char *search_dirs[3];
	search_dirs[0] = strndup(DATAROOTDIR"/pdf2laser/presets", PATH_MAX);
	search_dirs[1] = strndup(SYSCONFDIR"/pdf2laser/presets", PATH_MAX);

	size_t homedir_len = strnlen(getenv("HOME"), PATH_MAX) + strnlen(".pdf2laser/presets", 19) + 2;
	search_dirs[2] = calloc(homedir_len, sizeof(char));
	snprintf(search_dirs[2], homedir_len, "%s/.pdf2laser/presets", getenv("HOME"));

	size_t preset_file_count = 0;
	size_t preset_file_index = 0;
	struct dirent *directory_entry;

	for (size_t index = 0; index < 3; index += 1) {
		DIR *preset_dir = opendir(search_dirs[index]);
		if (preset_dir == NULL) {
			if (DEBUG) {
				perror("opendir failed");
			}
			continue;
		}
		while ((directory_entry = readdir(preset_dir))) {
			char *preset_file_path = append_directory(search_dirs[index], directory_entry->d_name);

			struct stat preset_file_stat;
			if (stat(preset_file_path, &preset_file_stat))
				goto pfd2laser_load_preset_counter_skip;

			if (!S_ISREG(preset_file_stat.st_mode))
				goto pfd2laser_load_preset_counter_skip;

			preset_file_count += 1;

		pfd2laser_load_preset_counter_skip:
			free(preset_file_path);
		}
		closedir(preset_dir);
	}

	*preset_files_count = preset_file_count;
	*preset_files = calloc(preset_file_count, sizeof(preset_file_t*));
	for (size_t index = 0; index < 3; index += 1) {
		DIR *preset_dir = opendir(search_dirs[index]);
		if (preset_dir == NULL) {
			if (DEBUG) {
				perror("opendir failed");
			}
			continue;
		}
		while ((directory_entry = readdir(preset_dir))) {
			char *preset_file_path = append_directory(search_dirs[index], directory_entry->d_name);

			struct stat preset_file_stat;
			if (stat(preset_file_path, &preset_file_stat))
				goto pfd2laser_load_preset_load_skip;

			if (!S_ISREG(preset_file_stat.st_mode))
				goto pfd2laser_load_preset_load_skip;

			if (preset_file_index < preset_file_count) {
				(*preset_files)[preset_file_index] = preset_file_create(preset_file_path);
				preset_file_index += 1;
			}

		pfd2laser_load_preset_load_skip:
			free(preset_file_path);
		}
		closedir(preset_dir);
	}

	for (size_t index = 0; index < 3; index += 1)
		free(search_dirs[index]);

	return  0;
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
	// Create temp working directory
	char *tmpdir_template = pdf2laser_format_string("%s/%s.XXXXXX", TMP_DIRECTORY, basename(argv[0]));
	char *tmpdir_name = mkdtemp(tmpdir_template);
	if (tmpdir_name == NULL) {
		perror("mkdtemp failed");
		return false;
	}

	// Load preset files
	preset_file_t **preset_files;
	size_t preset_files_count;
	pdf2laser_load_presets(&preset_files, &preset_files_count);

	// parse command line options
	print_job_t *print_job = print_job_create();
	pdf2laser_optparse(print_job, preset_files, preset_files_count, argc, argv);

	const char *source_filename = print_job->source_filename;
	char *source_basename = strndup(print_job->source_filename, FILENAME_NCHARS);
	char *source_basename_ptr = source_basename;
	source_basename = basename(source_basename);

	// If no job name is specified, use just the filename if there
	if (print_job->name == NULL) {
		print_job->name = strndup(source_basename, FILENAME_NCHARS);
	}

	// Report the settings on stdout
	printf("Configured values:\n%s\n", print_job_to_string(print_job));

	char *last_dot = strrchr(source_basename, '.');
	if (last_dot != NULL) {
		*last_dot = '\0';
	}
	char *target_base = pdf2laser_format_string("%s/%s", tmpdir_name, source_basename);

	free(source_basename_ptr);

	char *target_pdf = pdf2laser_format_string("%s.pdf", target_base);
	if (generate_pdf(source_filename, target_pdf)) {
		perror("Failed to clone pdf file");
		return -1;
	}

	char *target_ps = pdf2laser_format_string("%s.ps", target_base);
	if (generate_ps(target_pdf, target_ps)) {
		perror("Failed to generate ps file");
		return -1;
	}

	if (!print_job->debug) {
		if (unlink(target_pdf)) {
			perror("Error deleting pdf file");
			return -1;
		}
	}
	free(target_pdf);

	char *target_eps = pdf2laser_format_string("%s.eps", target_base);
	if (generate_eps(print_job, target_ps, target_eps)) {
		perror("Failed to generate eps file");
		return -1;
	}

	if (!print_job->debug) {
		if (unlink(target_ps)) {
			perror("Error deleting ps file");
			return -1;
		}
	}
	free(target_ps);

	char *target_bmp = pdf2laser_format_string("%s.bmp", target_base);
	char *target_vector = pdf2laser_format_string("%s.vector", target_base);
	const char * const raster_string = raster_mode_to_string(print_job->raster->mode);
	if (execute_ghostscript(print_job, target_eps, target_bmp, target_vector, raster_string)) {
		perror("Failed to execute ghostscript");
		return -1;
	}

	if (!print_job->debug) {
		if (unlink(target_eps)) {
			perror("Error deleting eps file");
			return -1;
		}
	}
	free(target_eps);

	char *target_pjl = pdf2laser_format_string("%s.pjl", target_base);
	if (generate_pjl(print_job, target_bmp, target_vector, target_pjl)) {
		perror("Failed to generate pjl file");
		return -1;
	}

	if (!print_job->debug) {
		if (unlink(target_bmp)) {
			perror("Error deleting bmp file");
			return -1;
		}
	}
	free(target_bmp);

	if (!print_job->debug) {
		if (unlink(target_vector)) {
			perror("Error deleting vector file");
			return -1;
		}
	}
	free(target_vector);

	free(target_base);

	if (printer_send(print_job, target_pjl)) {
		perror("Failed to send job to printer");
		return -1;
	}

	if (!print_job->debug) {
		if (unlink(target_pjl)) {
			perror("Error deleting pjl file");
			return -1;
		}
	}
	free(target_pjl);

	print_job_destroy(print_job);

	for (size_t index = 0; index < preset_files_count; index += 1) {
		preset_file_destroy(preset_files[index]);
	}

	if (!print_job->debug) {
		if (rmdir(tmpdir_name) == -1) {
			perror("Error deleting tmpdir");
			return -1;
		}
	}
	free(tmpdir_name);

	return 0;
}
