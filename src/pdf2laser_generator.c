#include "pdf2laser_generator.h"
#include <ghostscript/gserrors.h>   // for gs_error_type::gs_error_Quit
#include <ghostscript/iapi.h>       // for gsapi_delete_instance, gsapi_exit
#include <stdint.h>                 // for int32_t, uint8_t
#include <stdio.h>                  // for fprintf, fputc, printf, stderr, NULL
#include <stdlib.h>                 // for free, calloc
#include <string.h>                 // for strncmp
#include <strings.h>                // for strncasecmp
#include <sys/types.h>              // for ssize_t
#include <sys/sendfile.h>           // for sendfile
#include "pdf2laser_type.h"         // for print_job_t, raster_t
#include "pdf2laser_vector.h"       // for vector_t, point_t, point_compare
#include "pdf2laser_vector_list.h"  // for vector_list_t, vector_list_optimize

// bool debug = false;

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
static int32_t big_to_little_endian(uint8_t *position, size_t nbytes)
{
	int32_t result = 0;

	for (size_t i = 0; i < nbytes; i++)
		result += *(position + i) << (8 * i);

	return result;
}

/**
 *
 */
bool generate_ps(const char *target_pdf, const char *target_ps)
{
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
	snprintf(gs_argv[7], 1024, "-sOutputFile=%s", target_ps);

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
bool generate_eps(print_job_t *print_job, FILE *ps_file, FILE *eps_file)
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

				// if (print_job->flip)
				//	fprintf(eps_file, "%d 0 translate -1 1 scale\n", print_job->width);
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
 *
 */
bool generate_raster(print_job_t *print_job, FILE *pjl_file, FILE *bitmap_file)
{
	uint8_t bitmap_header[BITMAP_HEADER_NBYTES];
	char buf[102400];

	bool invert = false;

	int32_t basex = 0;
	int32_t basey = 0;

	int32_t h, d;

	int32_t passes = 1;
	if (print_job->raster->mode == 'c') {
		passes = 7;
	}

	/* Read in the bitmap header. */
	fread(bitmap_header, 1, BITMAP_HEADER_NBYTES, bitmap_file);

	/* Re-load width/height from bmp as it is possible that someone used
	 * setpagedevice or some such
	 */
	/* Bytes 18 - 21 are the bitmap width (little endian format). */
	int32_t width = big_to_little_endian(bitmap_header + 18, 4);

	/* Bytes 22 - 25 are the bitmap height (little endian format). */
	int32_t height = big_to_little_endian(bitmap_header + 22, 4);

	/* Bytes 10 - 13 base offset for the beginning of the bitmap data. */
	int32_t base_offset = big_to_little_endian(bitmap_header + 10, 4);

	if (print_job->raster->mode == 'c' || print_job->raster->mode == 'g') {
		/* colour/grey are byte per pixel power levels */
		h = width;
		/* BMP padded to 4 bytes per scan line */
		d = (h * 3 + 3) / 4 * 4;
	} else {
		/* mono */
		h = (width + 7) / 8;
		/* BMP padded to 4 bytes per scan line */
		d = (h + 3) / 4 * 4;
	}

	if (print_job->debug)
		printf("Width %d Height %d Bytes %d Line %d\n", width, height, h, d);

	/* Raster Orientation */
	fprintf(pjl_file, "\033*r0F");

	/* Raster power -- color and gray scaled before, but scale with the user provided power */
	fprintf(pjl_file, "\033&y%dP", print_job->raster->power);

	/* Raster speed */
	fprintf(pjl_file, "\033&z%dS", print_job->raster->speed);
	fprintf(pjl_file, "\033*r%dT", height);
	fprintf(pjl_file, "\033*r%dS", width);
	/* Raster compression */
	fprintf(pjl_file, "\033*b%dM", (print_job->raster->mode == 'c' || print_job->raster->mode == 'g') ? 7 : 2);
	/* Raster direction (1 = up) */
	fprintf(pjl_file, "\033&y1O");

	if (print_job->debug) {
		/* Output raster debug information */
		printf("Raster power=%d speed=%d\n",
			   ((print_job->raster->mode == 'c' || print_job->raster->mode == 'g') ?
				100 : print_job->raster->power),
			   print_job->raster->speed);
	}

	/* start at current position */
	fprintf(pjl_file, "\033*r1A");
	for (int32_t offx = 0; offx >= 0; offx -= width) {
		for (int32_t offy = 0; offy >= 0; offy -= height) {
			for (int32_t pass = 0; pass < passes; pass++) {
				// raster (basic)
				char dir = 0;

				fseek(bitmap_file, base_offset, SEEK_SET);
				for (int y = height - 1; y >= 0; y--) {
					int l;

					switch (print_job->raster->mode) {
					case 'c': {      // colour (passes)
						unsigned char *f = (unsigned char *) buf;
						unsigned char *t = (unsigned char *) buf;
						if (d > (int) sizeof (buf)) {
							perror("Too wide");
							return false;
						}
						l = fread ((char *)buf, 1, d, bitmap_file);
						if (l != d) {
							fprintf(stderr, "Bad bit data from gs %d/%d (y=%d)\n", l, d, y);
							return false;
						}
						while (l--) {
							// pack and pass check RGB
							int n = 0;
							int v = 0;
							int p = 0;
							int c = 0;
							for (c = 0; c < 3; c++) {
								if (*f > 240) {
									p |= (1 << c);
								} else {
									n++;
									v += *f;
								}
								f++;
							}
							if (n) {
								v /= n;
							} else {
								p = 0;
								v = 255;
							}
							if (p != pass) {
								v = 255;
							}
							*t++ = 255 - v;
						}
					}
						break;
					case 'g': {      // grey level
						/* BMP padded to 4 bytes per scan line */
						int d = (h + 3) / 4 * 4;
						if (d > (int) sizeof (buf)) {
							fprintf(stderr, "Too wide\n");
							return false;
						}
						l = fread((char *)buf, 1, d, bitmap_file);
						if (l != d) {
							fprintf (stderr, "Bad bit data from gs %d/%d (y=%d)\n", l, d, y);
							return false;
						}
						for (l = 0; l < h; l++) {
							if (invert)
								buf[l] = (uint8_t) buf[l];
							else
								buf[l] = (255 - (uint8_t)buf[l]);
						}
					}
						break;
					default: {       // mono
						static int i;
						if (i++==0)
							printf("mono\n");
						int d = (h + 3) / 4 * 4;  // BMP padded to 4 bytes per scan line
						if (d > (int) sizeof (buf)) {
							perror("Too wide");
							return false;
						}
						l = fread((char *) buf, 1, d, bitmap_file);
						if (l != d) {
							fprintf(stderr, "Bad bit data from gs %d/%d (y=%d)\n", l, d, y);
							return false;
						}
					}
					}

					if (print_job->raster->mode == 'c' || print_job->raster->mode == 'g') {
						for (l = 0; l < h; l++) {
							/* Raster value is multiplied by the
							 * power scale.
							 */
							buf[l] = (uint8_t)buf[l] * print_job->raster->power / 255;
						}
					}

					/* find left/right of data */
					for (l = 0; l < h && !buf[l]; l++)
						;

					if (l < h) {
						/* a line to print */
						int r;
						int n;
						unsigned char pack[sizeof (buf) * 5 / 4 + 1];
						for (r = h - 1; r > l && !buf[r]; r--)
							;

						r++;
						fprintf(pjl_file, "\033*p%dY", basey + offy + y);
						fprintf(pjl_file, "\033*p%dX", basex + offx +
								((print_job->raster->mode == 'c' || print_job->raster->mode == 'g') ? l : l * 8));
						if (dir) {
							fprintf(pjl_file, "\033*b%dA", -(r - l));
							// reverse bytes!
							for (n = 0; n < (r - l) / 2; n++){
								unsigned char t = buf[l + n];
								buf[l + n] = buf[r - n - 1];
								buf[r - n - 1] = t;
							}
						} else {
							fprintf(pjl_file, "\033*b%dA", (r - l));
						}
						dir = 1 - dir;
						// pack
						n = 0;
						while (l < r) {
							int p;
							for (p = l; p < r && p < l + 128 && buf[p] == buf[l]; p++) {
								;
							}
							if (p - l >= 2) {
								// run length
								pack[n++] = 257 - (p - l);
								pack[n++] = buf[l];
								l = p;
							} else {
								for (p = l;
									 p < r && p < l + 127 &&
										 (p + 1 == r || buf[p] !=
										  buf[p + 1]);
									 p++) {
									;
								}

								pack[n++] = p - l - 1;
								while (l < p) {
									pack[n++] = buf[l++];
								}
							}
						}
						fprintf(pjl_file, "\033*b%dW", (n + 7) / 8 * 8);
						r = 0;
						while (r < n)
							fputc(pack[r++], pjl_file);
						while (r & 7) {
							r++;
							fputc(0x80, pjl_file);
						}
					}
				}
			}
		}
	}

	fprintf(pjl_file, "\033*rC");       // end raster
	fputc(26, pjl_file);      // some end of file markers
	fputc(4, pjl_file);
		//}

	return true;
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
bool vectors_parse(print_job_t *print_job, FILE * const vector_file)
{
	vector_list_t *current_list = NULL;

	int32_t vector_count = 0;
	int32_t x_start, y_start, x_current, y_current;

	char *line = NULL;
	size_t length = 0;
	ssize_t length_read = 0;

	while ((length_read = getline(&line, &length, vector_file)) != -1) {

		switch (*line) {
		case 'P': {
			// Note: Colours are stored as blue, green, red in the vector file
			int32_t red, green, blue;
			sscanf(line, "P,%d,%d,%d", &blue, &green, &red);

			if (!red && green && !blue) {
				current_list = print_job->vectors[0];
				current_list->pass = 0;
				current_list->power = green;
			}
			else if (red && !green && !blue) {
				current_list = print_job->vectors[1];
				current_list->pass = 1;
				current_list->power = red;
			}
			else if (!red && !green && blue) {
				current_list = print_job->vectors[2];
				current_list->pass = 2;
				current_list->power = blue;
			}
			else {
				fprintf(stderr, "non-red/green/blue vector? %d,%d,%d\n", red, green, blue);
				exit(-1);
			}
			break;
		}
		case 'M': {
			// Start of new line. Implicitly sets current laser position.
			sscanf(line, "M%d,%d", &x_start, &y_start);
			x_current = x_start;
			y_current = y_start;
			break;
		}
		case 'L': {
			int32_t x_next, y_next;
			sscanf(line, "L%d,%d", &x_next, &y_next);
			vector_t *vector = vector_create(x_current, y_current, x_next, y_next);
			if (print_job->vector_optimize &&
				vector_list_contains(current_list, vector)) {
				free(vector);
			}
			else {
				vector_list_append(current_list, vector);
				vector_count += 1;
			}
			x_current = x_next;
			y_current = y_next;
			break;
		}
		case 'C': {
			// Closing statment from current point to starting point.
			vector_t *vector = vector_create(x_current, y_current, x_start, y_start);
			if (print_job->vector_optimize &&
				vector_list_contains(current_list, vector)) {
				free(vector);
			}
			else {
				vector_list_append(current_list, vector);
			}
			x_current = x_start;
			y_current = y_start;
			break;
		}
		case 'X':
			goto vector_parse_complete;
		default:
			fprintf(stderr, "Unknown command '%c'", *line);
			return NULL;
		}
	}

 vector_parse_complete:
	for (int i = 0 ; i < VECTOR_PASSES ; i++) {
		printf("Vector pass %d: segments=%d power=%d speed=%d\n",
			   i,
			   print_job->vectors[i]->length,
			   print_job->vectors[i]->power,
			   print_job->vectors[i]->speed);
		vector_list_stats(print_job->vectors[i]);
	}

	return true;
}

static void output_vector(vector_list_t *list, FILE * const pjl_file)
{
	int32_t current_x = 0;
	int32_t current_y = 0;
	vector_t *vector = list->head;
	while (vector) {
		if (point_compare(vector->start, &(point_t){ current_x, current_y }) != 0) {
			// Stop the laser; we need to transit and then start the laser as
			// we go to the next point.  Note initial ";"
			fprintf(pjl_file, ";PU%d,%d;PD%d,%d", vector->start->y, vector->start->x, vector->end->y, vector->end->x);
		}
		else {
			// This is the continuation of a line, so just add additional
			// points
			fprintf(pjl_file, ",%d,%d", vector->end->y, vector->end->x);
		}

		// Changing power on the fly is not supported for now
		// \todo: Check v->power and adjust ZS, XR, etc

		// Move to the next vector, updating our current point
		current_x = vector->end->x;
		current_y = vector->end->y;
		vector = vector->next;
	}

	// Stop the laser (note initial ";")
	fprintf(pjl_file, ";PU;");
}


bool generate_vector(print_job_t *print_job, FILE * const pjl_file, FILE * const vector_file)
{
	// this mutates vectors parser in print_job
	vectors_parse(print_job, vector_file);

	fprintf(pjl_file, "IN;");
	fprintf(pjl_file, "XR%04d;", print_job->vector_frequency);

	for (int i = 0; i < VECTOR_PASSES; i++) {
		if (print_job->vector_optimize) {
			vector_list_t *vl = print_job->vectors[i];
			print_job->vectors[i] = vector_list_optimize(vl);
			free(vl);
		}

		fprintf(pjl_file, "YP%03d;", print_job->vectors[i]->power);
		fprintf(pjl_file, "ZS%03d", print_job->vectors[i]->speed); // note: no ";"

		output_vector(print_job->vectors[i], pjl_file);
	}

	fprintf(pjl_file, "\033%%0B"); // end HLGL
	fprintf(pjl_file, "\033%%1BPU"); // start HLGL, pen up?

	return true;
}


/**
 *
 */
bool generate_pjl(print_job_t *print_job, FILE *bitmap_file, FILE *pjl_file, FILE *vector_file)
{
	/* Print the printer job language header. */
	fprintf(pjl_file, "%s", "\033%-12345X@PJL COMMENT *Job Start*\r\n");
	fprintf(pjl_file, "@PJL JOB NAME=%s\r\n", print_job->name);
	fprintf(pjl_file, "@PJL ENTER LANGUAGE=PCL\r\n");
	/* Set autofocus on or off. */
	fprintf(pjl_file, "\033&y%dA", print_job->focus);
	/* Left (long-edge) offset registration.  Adjusts the position of the
	 * logical page across the width of the page.
	 */
	fprintf(pjl_file, "\033&l0U");
	/* Top (short-edge) offset registration.  Adjusts the position of the
	 * logical page across the length of the page.
	 */
	fprintf(pjl_file, "\033&l0Z");

	/* Resolution of the print. */
	fprintf(pjl_file, "\033&u%dD", print_job->raster->resolution);
	/* X position = 0 */
	fprintf(pjl_file, "\033*p0X");
	/* Y position = 0 */
	fprintf(pjl_file, "\033*p0Y");
	/* PCL resolution. */
	fprintf(pjl_file, "\033*t%dR", print_job->raster->resolution);

	/* If raster power is enabled and raster mode is not 'n' then add that
	 * information to the print job.
	 */
	if (print_job->raster->power && print_job->raster->mode != 'n') {
		/* FIXME unknown purpose. */
		fprintf(pjl_file, "\033&y0C");

		/* We're going to perform a raster print. */
		generate_raster(print_job, pjl_file, bitmap_file);
	}

	/* If vector power is > 0 then add vector information to the print job. */
	fprintf(pjl_file, "\033E@PJL ENTER LANGUAGE=PCL\r\n");
	/* Page Orientation */
	fprintf(pjl_file, "\033*r0F");
	fprintf(pjl_file, "\033*r%dT", print_job->height);
	fprintf(pjl_file, "\033*r%dS", print_job->width);
	fprintf(pjl_file, "\033*r1A");
	fprintf(pjl_file, "\033*rC");
	fprintf(pjl_file, "\033%%1B");

	/* We're going to perform a vector print. */
	generate_vector(print_job, pjl_file, vector_file);

	/* Footer for printer job language. */

	/* Reset */
	fprintf(pjl_file, "\033E");

	/* Exit language. */
	//fprintf(pjl_file, "\033%%-12345X");
	fprintf(pjl_file, "%s", "\033%-12345X@PJL COMMENT *Job End*\r\n");

	/* End job. */
	fprintf(pjl_file, "@PJL EOJ\r\n");

	/* Pad out the remainder of the file with 0 characters. */
	// for(int i = 0; i < 4096; i++)
	//	fputc(0, pjl_file);

	return true;
}
