/** \file
 * Live laser test.
 *
 * HPGL spec: http://www.piclist.com/techref/language/hpgl/commands.htm
 *
 * Normal commands:
 * PD[x1,y1[,x2,y2...]]; -- pen down and list of coordinates
 * PU[x1,y1[,x2,y2...]]; -- pen up and list of coordinates
 *
 * Special commands:
 * XR == vector frequency
 * YP == vectory power
 * ZS == vector speed
 *
 */
/*
 * Copyright 2011 Trammell Hudson <hudson@osresearch.net>
 *
 * Portions
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
 */
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>


typedef struct
{
	FILE * file;
	const char * host;
	const char * title;
	const char * queue;
	const char * user;
	const char * job_name;
	size_t job_size;
	int auto_focus;
	uint32_t resolution;
	uint32_t width;
	uint32_t height;
} printer_job_t;


void
vector_moveto(
	printer_job_t * const job,
	uint32_t x,
	uint32_t y,
	int pen
)
{
	fprintf(job->file, "P%c%d%d;",
		pen ? 'D' : 'U',
		x,
		y
	);
}


void
vector_param(
	printer_job_t * const job,
	int freq,
	int power,
	int speed
)
{
	fprintf(job->file,
		"XR%04d;YP%03d;ZS%03d;",
		freq,
		power,
		speed
	);
}


void
vector_init(
	printer_job_t * const job
)
{
	/* add vector information to the print job. */
        fprintf(job->file, "\eE@PJL ENTER LANGUAGE=PCL\r\n");

        /* Page Orientation */
        fprintf(job->file, "\e*r0F");
        fprintf(job->file, "\e*r%dT", job->height);
        fprintf(job->file, "\e*r%dS", job->width);
        fprintf(job->file, "\e*r1A");
        fprintf(job->file, "\e*rC");
        fprintf(job->file, "\e%%1B");

	// We are now in HPGL mode 
	fprintf(job->file, "IN;");
}


void
vector_end(
	printer_job_t * const job
)
{
	fprintf(job->file, "\e%%0B"); // end HLGL
}


/**
 *
 */
static void
printer_header(
	printer_job_t * const job
)
{
    /* Print the printer job language header. */
    fprintf(job->file, "\e%%-12345X@PJL JOB NAME=%s\r\n", job->title);
    fprintf(job->file, "\eE@PJL ENTER LANGUAGE=PCL\r\n");
    /* Set autofocus on or off. */
    fprintf(job->file, "\e&y%dA", job->auto_focus);
    /* Left (long-edge) offset registration.  Adjusts the position of the
     * logical page across the width of the page.
     */
    fprintf(job->file, "\e&l0U");
    /* Top (short-edge) offset registration.  Adjusts the position of the
     * logical page across the length of the page.
     */
    fprintf(job->file, "\e&l0Z");

    /* Resolution of the print. */
    fprintf(job->file, "\e&u%dD", job->resolution);
    /* X position = 0 */
    fprintf(job->file, "\e*p0X");
    /* Y position = 0 */
    fprintf(job->file, "\e*p0Y");
    /* PCL resolution. */
    fprintf(job->file, "\e*t%dR", job->resolution);

#if 0
    /* If raster power is enabled and raster mode is not 'n' then add that
     * information to the print job.
     */
    if (raster_power && raster_mode != 'n') {

        /* FIXME unknown purpose. */
        fprintf(pjl_file, "\e&y0C");

        /* We're going to perform a raster print. */
        generate_raster(pjl_file, bitmap_file);
    }
#endif

}


/** Send footer for printer job language.
 */
static void
printer_footer(
	printer_job_t * const job
)
{
	/* Reset */
	fprintf(job->file, "\eE");
	/* Exit language. */
	fprintf(job->file, "\e%%-12345X");
	/* End job. */
	fprintf(job->file, "@PJL EOJ \r\n");

	/* Pad out the remainder of the file with 0 characters. */
	int i;
	for(i = 0; i < 4096; i++)
		fputc('\0', job->file);

	fflush(job->file);
}


/**
 * Connect to a printer.
 *
 * @param host The hostname or IP address of the printer to connect to.
 * @param timeout The number of seconds to wait before timing out on the
 * connect operation.
 * @return A socket descriptor to the printer.
 */
static FILE *
tcp_connect(const char *host, const int timeout)
{
	int i;

	for (i = 0; i < timeout; i++)
	{
		struct addrinfo *res;
		struct addrinfo *addr;
		struct addrinfo base = {
			.ai_family	= PF_UNSPEC,
			.ai_socktype	= SOCK_STREAM,
		};
		int error_code = getaddrinfo(host, "printer", &base, &res);

		/* Set an alarm to go off if the program has gone out to lunch. */
		alarm(10);

		/* If getaddrinfo did not return an error code then we attempt to
		 * connect to the printer and establish a socket.
		 */
		if (error_code)
		{
			/* Sleep for a second then try again. */
			sleep(1);
			continue;
		}

		for (addr = res; addr; addr = addr->ai_next)
		{
			const struct sockaddr_in * addr_in
				= (void*) addr->ai_addr;

			if (!addr_in)
				continue;

			fprintf(stderr,
				"trying to connect to %s:%d\n",
				inet_ntoa(addr_in->sin_addr),
				ntohs(addr_in->sin_port)
		       );

			const int fd = socket(
				addr->ai_family,
				addr->ai_socktype,
				addr->ai_protocol
			);

			if (fd < 0)
				continue;

			int rc = connect(
				fd,
				addr->ai_addr,
				addr->ai_addrlen
			);

			if (rc < 0)
			{
				close(fd);
				continue;
			}

			// Found it and connected
			alarm(0);
			freeaddrinfo(res);
			return fdopen(fd, "rw");
		}

		// Didn't find anything this time; try again
		freeaddrinfo(res);
	}

	fprintf(stderr, "Cannot connect to %s\n", host);
	alarm(0);
	return NULL;
}


/**
 * Disconnect from a printer.
 *
 * @param socket_descriptor the descriptor of the printer that is to be
 * disconnected from.
 * @return True if the printer connection was successfully closed, false otherwise.
 */
static bool
tcp_disconnect(FILE * file)
{
	int fd = fileno(file);
	int rc = close(fd);

	if (rc == 0)
		return true;

	perror("close failed");
	return false;
}


static void
printer_disconnect(
	printer_job_t * const job
)
{
	tcp_disconnect(job->file);
	free(job);
}


static int
printer_read(
	printer_job_t * const job
)
{
	// ensure that all output has been sent
	fflush(job->file);
	const int fd = fileno(job->file);
	printf("waiting on fd %d\n", fd);
	uint8_t result;
	ssize_t rc = read(fd, &result, sizeof(result));

	// Success == 0 byte
	if (rc == sizeof(result) && result == 0)
		return 1;

	if (rc != sizeof(result))
		perror("printer read failed");
	else
		fprintf(stderr, "printer returned failure code %02x\n", result);

	return 0;
}


/**
 *
 */
static printer_job_t *
printer_connect(
	const char * const host
)
{
	printer_job_t * const job = calloc(1, sizeof(*job));
	if (!job)
		return NULL;

	*job = (printer_job_t) {
		.host		= strdup(host),
		.job_name	= "live",
		.job_size	= 1 << 20, // fake 1 MB for now
		.queue		= "",
		.user		= "user",
		.auto_focus	= 0,
		.resolution	= 1200,
		.width		= 8,
		.height		= 8,
	};

	char localhost[128] = "";

	gethostname(localhost, sizeof(localhost));
	char *d = strchr(localhost, '.');
	if (d)
	    *d = 0;

	/* Connect to the printer. */
	job->file = tcp_connect(job->host, 60);
	if (!job->file)
		return NULL;

	// talk to printer
	fprintf(job->file, "\002%s\n", job->queue);
	if (!printer_read(job))
		return NULL;

	fprintf(job->file, "P%s\n", job->user);
	fprintf(job->file, "J%s\n", job->title);
	fprintf(job->file, "ldfA%s%s\n", job->job_name, localhost);
	fprintf(job->file, "UdfA%s%s\n", job->job_name, localhost);
	fprintf(job->file, "N%s\n", job->title);
	fprintf(job->file, "\002%zu cfA%s%s\n",
		strlen(localhost) + 2, // the length of the H command
		job->job_name,
		localhost
	);
	if (!printer_read(job))
		return NULL;

	fprintf(job->file, "H%s\n", localhost);
	if (!printer_read(job))
		return NULL;

	fprintf(job->file, "\003%zu dfA%s%s\n",
		job->job_size,
		job->job_name,
		localhost
	);
	if (!printer_read(job))
		return NULL;

	return job;
}


int main(void)
{
	printer_job_t * const job = printer_connect("192.168.3.4");

	printf("connected\n");

	printer_header(job);
	vector_init(job);
	vector_param(job, 5000, 100, 5);

	const uint32_t points[][2] = {
		{ 0, 0 },
		{ 1200, 0 },
		{ 1200, 1200 },
		{ 0, 1200 },
	};

	const unsigned num_points = sizeof(points) / sizeof(*points);

	unsigned i = 0;
	while (1)
	{
		printf("sending point %d\n", i);

		vector_moveto(job, 1, points[i][0], points[i][1]);
		fflush(job->file);
		i = (i + 1) % num_points;
		getchar();
	}

	vector_end(job);
	printer_footer(job);
	printer_disconnect(job);
}
