#include "pdf2laser_generator.h"

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
int big_to_little_endian(uint8_t *position, int nbytes)
{
	int result = 0;

	for (int i = 0; i < nbytes; i++)
		result += *(position + i) << (8 * i);

	return result;
}

/**
 *
 */
bool generate_raster(FILE *pjl_file, FILE *bitmap_file)
{
	const int invert = 0;
	int h;
	int d;
	int basex = 0;
	int basey = 0;
	int repeat;

	uint8_t bitmap_header[BITMAP_HEADER_NBYTES];

	if (x_center)
		basex = x_center - width / 2;

	if (y_center)
		basey = y_center - height / 2;

	if (basex < 0)
		basex = 0;

	if (basey < 0)
		basey = 0;

	// rasterises
	basex = basex * resolution / POINTS_PER_INCH;
	basey = basey * resolution / POINTS_PER_INCH;

	repeat = raster_repeat;
	while (repeat--) {
		/* repeated (over printed) */
		long base_offset;
		int passes = 1;
		if (print_raster_mode == 'c') {
			passes = 7;
		}

		/* Read in the bitmap header. */
		fread(bitmap_header, 1, BITMAP_HEADER_NBYTES, bitmap_file);

		/* Re-load width/height from bmp as it is possible that someone used
		 * setpagedevice or some such
		 */
		/* Bytes 18 - 21 are the bitmap width (little endian format). */
		width = big_to_little_endian(bitmap_header + 18, 4);

		/* Bytes 22 - 25 are the bitmap height (little endian format). */
		height = big_to_little_endian(bitmap_header + 22, 4);

		/* Bytes 10 - 13 base offset for the beginning of the bitmap data. */
		base_offset = big_to_little_endian(bitmap_header + 10, 4);

		if (print_raster_mode == 'c' || print_raster_mode == 'g') {
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

		if (debug)
			printf("Width %d Height %d Bytes %d Line %d\n", width, height, h, d);

		/* Raster Orientation */
		fprintf(pjl_file, "\033*r0F");

		/* Raster power -- color and gray scaled before, but scale with the user provided power */
		fprintf(pjl_file, "\033&y%dP", raster_power);

		/* Raster speed */
		fprintf(pjl_file, "\033&z%dS", raster_speed);
		fprintf(pjl_file, "\033*r%dT", height * y_repeat);
		fprintf(pjl_file, "\033*r%dS", width * x_repeat);
		/* Raster compression */
		fprintf(pjl_file, "\033*b%dM", (print_raster_mode == 'c' || print_raster_mode == 'g')
				? 7 : 2);
		/* Raster direction (1 = up) */
		fprintf(pjl_file, "\033&y1O");

		if (debug) {
			/* Output raster debug information */
			printf("Raster power=%d speed=%d\n",
				   ((print_raster_mode == 'c' || print_raster_mode == 'g') ?
					100 : raster_power),
				   raster_speed);
		}

		/* start at current position */
		fprintf(pjl_file, "\033*r1A");
		for (int offx = width * (x_repeat - 1); offx >= 0; offx -= width) {
			for (int offy = height * (y_repeat - 1); offy >= 0; offy -= height) {
				for (int pass = 0; pass < passes; pass++) {
					// raster (basic)
					char dir = 0;

					fseek(bitmap_file, base_offset, SEEK_SET);
					for (int y = height - 1; y >= 0; y--) {
						int l;

						switch (print_raster_mode) {
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

						if (print_raster_mode == 'c' || print_raster_mode == 'g') {
							for (l = 0; l < h; l++) {
								/* Raster value is multiplied by the
								 * power scale.
								 */
								buf[l] = (uint8_t)buf[l] * raster_power / 255;
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
									((print_raster_mode == 'c' || print_raster_mode == 'g') ? l : l * 8));
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
	}

	return true;
}

void vector_stats(vector_t *v)
{
	int lx = 0;
	int ly = 0;
	long cut_len_sum = 0;
	int cuts = 0;

	long transit_len_sum = 0;
	int transits = 0;

	while (v) {
		long t_dx = lx - v->x1;
		long t_dy = ly - v->y1;

		long transit_len = sqrt(t_dx * t_dx + t_dy * t_dy);
		if (transit_len != 0) {
			transits++;
			transit_len_sum += transit_len;

			if (0)
				fprintf(stderr, "mov %8u %8u -> %8u %8u\n", lx, ly, v->x1, v->y1);
		}

		long c_dx = v->x1 - v->x2;
		long c_dy = v->y1 - v->y2;

		long cut_len = sqrt(c_dx*c_dx + c_dy*c_dy);
		if (cut_len != 0) {
			cuts++;
			cut_len_sum += cut_len;

			if (0)
				fprintf(stderr, "cut %8u %8u -> %8u %8u\n", v->x1, v->y1, v->x2, v->y2);
		}

		// Advance the point
		lx = v->x2;
		ly = v->y2;
		v = v->next;
	}

	printf("Cuts: %u len %lu\n", cuts, cut_len_sum);
	printf("Move: %u len %lu\n", transits, transit_len_sum);
}


static void vector_create(vectors_t * const vectors,
						  int power,
						  int x1,
						  int y1,
						  int x2,
						  int y2)
{
	// Find the end of the list and, if vector optimization is
	// turned on, check for duplicates
	vector_t ** iter = &vectors->vectors;
	while (*iter) {
		vector_t * const p = *iter;

		if (do_vector_optimize) {
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
static vectors_t *vectors_parse(FILE * const vector_file)
{
	vectors_t * const vectors = calloc(VECTOR_PASSES, sizeof(*vectors));
	int mx = 0, my = 0;
	int lx = 0, ly = 0;
	int pass = 0;
	int power = 100;
	int count = 0;

	char buf[256];

	while (fgets(buf, sizeof(buf), vector_file)) {
		//fprintf(stderr, "read '%s'\n", buf);
		const char cmd = buf[0];
		int x, y;

		switch (cmd) {
		case 'P': {
			// note that they will be in bgr order in the file
			int r, g, b;
			sscanf(buf + 1, ",%d,%d,%d", &b, &g, &r);
			if (r == 0 && g != 0 && b == 0) {
				pass = 0;
				power = g;
			} else
				if (r != 0 && g == 0 && b == 0) {
					pass = 1;
					power = r;
				} else {
					if (r == 0 && g == 0 && b != 0) {
						pass = 2;
						power = b;
					} else {
						fprintf(stderr, "non-red/green/blue vector? %d,%d,%d\n", r, g, b);
						exit(-1);
					}
				}
			break;
		}
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
			vector_create(&vectors[pass], power, lx, ly, x, y);
			count++;
			lx = x;
			ly = y;
			break;
		case 'C':
			// Closing segment from the current point
			// back to the starting point
			vector_create(&vectors[pass], power, lx, ly, mx, my);
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
	printf("read %u segments\n", count);
	for (int i = 0 ; i < VECTOR_PASSES ; i++) {
		printf("Vector pass %d: power=%d speed=%d\n",
			   i,
			   vector_power[i],
			   vector_speed[i]);
		vector_stats(vectors[i].vectors);
	}

	return vectors;
}


/** Find the closest vector to a given point and remove it from the list.
 *
 * This might reverse a vector if it is closest to draw it in reverse
 * order.
 */
vector_t *vector_find_closest(vector_t *v, const int cx, const int cy)
{
	long best_dist = LONG_MAX;
	vector_t * best = NULL;
	int do_reverse = 0;

	while (v) {
		long dx1 = cx - v->x1;
		long dy1 = cy - v->y1;
		long dist1 = dx1*dx1 + dy1*dy1;

		if (dist1 < best_dist) {
			best = v;
			best_dist = dist1;
			do_reverse = 0;
		}

		long dx2 = cx - v->x2;
		long dy2 = cy - v->y2;
		long dist2 = dx2*dx2 + dy2*dy2;
		if (dist2 < best_dist) {
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
	if (do_reverse) {
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
static int vector_optimize(vectors_t * const vectors)
{
	int cx = 0;
	int cy = 0;

	vector_t *vs = NULL;
	vector_t *vs_tail = NULL;

	while (vectors->vectors) {
		vector_t *v = vector_find_closest(vectors->vectors, cx, cy);

		if (!vs) {
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


static void output_vector(FILE * const pjl_file, const vector_t *v)
{
	int lx = 0;
	int ly = 0;

	while (v) {
		if (v->x1 != lx || v->y1 != ly) {
			// Stop the laser; we need to transit
			// and then start the laser as we go to
			// the next point.  Note initial ";"
			fprintf(pjl_file, ";PU%d,%d;PD%d,%d", v->y1, v->x1, v->y2, v->x2);
		} else {
			// This is the continuation of a line, so
			// just add additional points
			fprintf(pjl_file, ",%d,%d", v->y2, v->x2);
		}

		// Changing power on the fly is not supported for now
		// \todo: Check v->power and adjust ZS, XR, etc

		// Move to the next vector, updating our current point
		lx = v->x2;
		ly = v->y2;
		v = v->next;
	}

	// Stop the laser (note initial ";")
	fprintf(pjl_file, ";PU;");
}


static bool generate_vector(FILE * const pjl_file, FILE * const vector_file)
{
	vectors_t * const vectors = vectors_parse(vector_file);

	fprintf(pjl_file, "IN;");
	fprintf(pjl_file, "XR%04d;", vector_freq);

	// \note: step and repeat is no longer supported

	for (int i = 0; i < VECTOR_PASSES; i++) {
		if (do_vector_optimize)
			vector_optimize(&vectors[i]);

		const vector_t * v = vectors[i].vectors;

		fprintf(pjl_file, "YP%03d;", vector_power[i]);
		fprintf(pjl_file, "ZS%03d", vector_speed[i]); // note: no ";"
		output_vector(pjl_file, v);
	}

	fprintf(pjl_file, "\033%%0B"); // end HLGL
	fprintf(pjl_file, "\033%%1BPU"); // start HLGL, pen up?

	return true;
}


/**
 *
 */
static bool generate_pjl(FILE *bitmap_file, FILE *pjl_file, FILE *vector_file)
{
	/* Print the printer job language header. */
	fprintf(pjl_file, "\033%%-12345X@PJL JOB NAME=%s\r\n", job_title);
	fprintf(pjl_file, "\033E@PJL ENTER LANGUAGE=PCL\r\n");
	/* Set autofocus on or off. */
	fprintf(pjl_file, "\033&y%dA", focus);
	/* Left (long-edge) offset registration.  Adjusts the position of the
	 * logical page across the width of the page.
	 */
	fprintf(pjl_file, "\033&l0U");
	/* Top (short-edge) offset registration.  Adjusts the position of the
	 * logical page across the length of the page.
	 */
	fprintf(pjl_file, "\033&l0Z");

	/* Resolution of the print. */
	fprintf(pjl_file, "\033&u%dD", resolution);
	/* X position = 0 */
	fprintf(pjl_file, "\033*p0X");
	/* Y position = 0 */
	fprintf(pjl_file, "\033*p0Y");
	/* PCL resolution. */
	fprintf(pjl_file, "\033*t%dR", resolution);

	/* If raster power is enabled and raster mode is not 'n' then add that
	 * information to the print job.
	 */
	if (raster_power && print_raster_mode != 'n') {
		/* FIXME unknown purpose. */
		fprintf(pjl_file, "\033&y0C");

		/* We're going to perform a raster print. */
		generate_raster(pjl_file, bitmap_file);
	}

	/* If vector power is > 0 then add vector information to the print job. */
	fprintf(pjl_file, "\033E@PJL ENTER LANGUAGE=PCL\r\n");
	/* Page Orientation */
	fprintf(pjl_file, "\033*r0F");
	fprintf(pjl_file, "\033*r%dT", height * y_repeat);
	fprintf(pjl_file, "\033*r%dS", width * x_repeat);
	fprintf(pjl_file, "\033*r1A");
	fprintf(pjl_file, "\033*rC");
	fprintf(pjl_file, "\033%%1B");

	/* We're going to perform a vector print. */
	generate_vector(pjl_file, vector_file);

	/* Footer for printer job language. */
	/* Reset */
	fprintf(pjl_file, "\033E");
	/* Exit language. */
	fprintf(pjl_file, "\033%%-12345X");
	/* End job. */
	fprintf(pjl_file, "@PJL EOJ \r\n");
	/* Pad out the remainder of the file with 0 characters. */
	for(int i = 0; i < 4096; i++)
		fputc(0, pjl_file);

	return true;
}
