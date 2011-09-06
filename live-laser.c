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
#include <stdarg.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>
#include <cv.h>
#include <highgui.h>


typedef struct
{
	int fd;
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
	size_t sent_bytes;
} printer_job_t;


void
printer_send_raw(
	printer_job_t * const job,
	const void * const buf,
	const size_t len
)
{
#if 0
	unsigned i;
	fprintf(stderr, "sending %zu bytes:", len);
	for(i = 0 ; i < len ; i++)
	{
		char c = buf[i];
		fprintf(stderr, "%c", isprint(c) ? c : '.');
	}

	for(i = 0 ; i < len ; i++)
		fprintf(stderr, " %02x", (uint8_t) buf[i]);
	fprintf(stderr, "\n");
#else
	//write(1, buf, len);
#endif
	if (job->sent_bytes > job->job_size - 64)
	{
		fprintf(stderr, "Too many bytes!  We're done\n");
		return;
	}

	// epilog.c sends the nul at the end?
	ssize_t rc = write(job->fd, buf, len);
	if (rc != (ssize_t) len)
	{
		perror("short write");
		abort();
	}

	job->sent_bytes += len;
}


void
printer_send(
	printer_job_t * const job,
	const char * const fmt,
	...
)
{
	static char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	ssize_t len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	printer_send_raw(job, buf, len);
}


void
vector_moveto(
	printer_job_t * const job,
	int pen,
	uint32_t x,
	uint32_t y
)
{
	printer_send(job, "P%c%d,%d;",
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
	printer_send(job,
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
        printer_send(job, "\eE@PJL ENTER LANGUAGE=PCL\r\n");

        /* Page Orientation */
        printer_send(job, "\e*r0F");
        printer_send(job, "\e*r%dT", job->height);
        printer_send(job, "\e*r%dS", job->width);
        printer_send(job, "\e*r1A");
        printer_send(job, "\e*rC");
        printer_send(job, "\e%%1B");

	// We are now in HPGL mode 
	printer_send(job, "IN;");
}


void
vector_end(
	printer_job_t * const job
)
{
	printer_send(job, "\e%%0B"); // end HLGL
	printer_send(job, "\e%%1BPU");  // start HLGL, and pen up, end
}



/** vector doesn't work without a fake raster component */
static void
raster_header(
	printer_job_t * const job
)
{
	/* Unknown purpose */
	printer_send(job, "\e&y0C");
        /* Raster Orientation */
        printer_send(job, "\e*r0F");
        /* Raster power */
        printer_send(job, "\e&y%dP", 100);

        /* Raster speed */
        printer_send(job, "\e&z%dS", 100);
        printer_send(job, "\e*r%dT", job->height);
        printer_send(job, "\e*r%dS", job->width);

        /* Raster compression */
/*
        fprintf(pjl_file, "\e*b%dM", (raster_mode == 'c' || raster_mode == 'g')
                ? 7 : 2);
*/
	printer_send(job, "\e*b7M");

        /* Raster direction (1 = up) */
        printer_send(job, "\e&y1O");
}

static void
raster_footer(
	printer_job_t * const job
)
{
        printer_send(job, "\e*rC%c%c",
		(char) 26,
		(char) 4
	);
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
    printer_send(job, "\e%%-12345X@PJL JOB NAME=%s\r\n", job->title);
    printer_send(job, "\eE@PJL ENTER LANGUAGE=PCL\r\n");
    /* Set autofocus on or off. */
    printer_send(job, "\e&y%dA", job->auto_focus);
    /* Left (long-edge) offset registration.  Adjusts the position of the
     * logical page across the width of the page.
     */
    printer_send(job, "\e&l0U");
    /* Top (short-edge) offset registration.  Adjusts the position of the
     * logical page across the length of the page.
     */
    printer_send(job, "\e&l0Z");

    /* Resolution of the print. */
    printer_send(job, "\e&u%dD", job->resolution);
    /* X position = 0 */
    printer_send(job, "\e*p0X");
    /* Y position = 0 */
    printer_send(job, "\e*p0Y");
    /* PCL resolution. */
    printer_send(job, "\e*t%dR", job->resolution);

	// vector only mode still requires an empty raster section
	raster_header(job);
	raster_footer(job);

#if 0
    /* If raster power is enabled and raster mode is not 'n' then add that
     * information to the print job.
     */
    if (raster_power && raster_mode != 'n') {

        /* FIXME unknown purpose. */
        printer_send(job, "\e&y0C");

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
	printer_send(job, "\eE");
	/* Exit language and end job. */
	printer_send(job, "\e%%-12345X@PJL EOJ \r\n");

	fprintf(stderr, "sent %zu bytes / %zu\n", job->sent_bytes, job->job_size);

	/* Pad out the remainder of the file with 0 characters. */
	size_t i;
	size_t remaining = job->job_size - job->sent_bytes;
	fprintf(stderr, "sending %zu pad bytes\n", remaining);
	job->sent_bytes = 0;

	void * buf = calloc(1, remaining + 4096);
	printer_send_raw(job, buf, remaining + 4096);
}


/**
 * Connect to a printer.
 *
 * @param host The hostname or IP address of the printer to connect to.
 * @param timeout The number of seconds to wait before timing out on the
 * connect operation.
 * @return A socket descriptor to the printer.
 */
static int
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
			return fd;
		}

		// Didn't find anything this time; try again
		freeaddrinfo(res);
	}

	fprintf(stderr, "Cannot connect to %s\n", host);
	alarm(0);
	return -1;
}


/**
 * Disconnect from a printer.
 *
 * @param socket_descriptor the descriptor of the printer that is to be
 * disconnected from.
 * @return True if the printer connection was successfully closed, false otherwise.
 */
static bool
tcp_disconnect(int fd)
{
	fprintf(stderr, "closing socket\n");
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
	tcp_disconnect(job->fd);
	free(job);
}


static int
printer_read(
	printer_job_t * const job
)
{
	// ensure that all output has been sent
	fprintf(stderr, "waiting on fd %d\n", job->fd);
	int8_t result = -1;
	ssize_t rc = read(job->fd, &result, sizeof(result));

	fprintf(stderr, "rc=%zd value=%d\n", rc, result);

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

	const uint32_t dpi = 150;

	*job = (printer_job_t) {
		.host		= strdup(host),
		.job_name	= "live.pdf",
		.job_size	= 64 << 10, // fake 64 MB for now
		.title		= "live-test",
		.queue		= "",
		.user		= "user",
		.auto_focus	= 0,
		.resolution	= dpi,
		.width		= 8 * dpi,
		.height		= 8 * dpi,
	};

	char localhost[128] = "";

	gethostname(localhost, sizeof(localhost));
	char *d = strchr(localhost, '.');
	if (d)
	    *d = 0;

	/* Connect to the printer. */
	job->fd = tcp_connect(job->host, 60);
	if (job->fd < 0)
		return NULL;

	// talk to printer
	printer_send(job, "\002%s\n", job->queue);
	if (!printer_read(job))
		return NULL;

/*
	// due to off-by-one errors, the epilog.c code never sends these
	printer_send(job, "P%s\n", job->user);
	printer_send(job, "J%s\n", job->title);
	printer_send(job, "ldfA%s:%s\n", job->job_name, localhost);
	printer_send(job, "UdfA%s:%s\n", job->job_name, localhost);
	printer_send(job, "N%s\n", job->title);
*/
	printer_send(job, "\002%zu cfA%s%s\n",
		strlen(localhost) + 2, // the length of the H command
		job->job_name,
		localhost
	);
	if (!printer_read(job))
		return NULL;

	printer_send(job, "H%s\n", localhost);
	char nul = '\0';
	printer_send_raw(job, &nul, 1);
	if (!printer_read(job))
		return NULL;

	printer_send(job, "\003%zu dfA%s%s\n",
		job->job_size,
		job->job_name,
		localhost
	);
	if (!printer_read(job))
		return NULL;

	job->sent_bytes = 0;
	return job;
}


static void
mouse_handler(
	int event,
	int x,
	int y,
	int flags,
	void * job_ptr
)
{
	static int last_down;

	printer_job_t * const job = job_ptr;
	switch (event)
	{
	case CV_EVENT_MOUSEMOVE:
		if (flags & CV_EVENT_FLAG_CTRLKEY)
		{
			// If we're not already with the laser on,
			// do the first move with out firing.
			vector_moveto(job, last_down, x*2, y*2);
			last_down = 1;
			printf("%d, %d, %d\n", flags, x, y);
		} else
		if (last_down) {
			// Force a non-laser movement
			vector_moveto(job, 0, x*2, y*2);
			last_down = 0;
		}

		break;
	case CV_EVENT_LBUTTONDOWN:
		vector_moveto(job, 0, x*2, y*2);
		break;
	default:
		printf("event %d\n", event);
		break;
	}
}


int main(void)
{
	printer_job_t * const job = printer_connect("192.168.3.4");
	if (!job)
		return -1;

	fprintf(stderr, "connected\n");

	printer_header(job);
	job->sent_bytes = 0;

	vector_init(job);
	vector_param(job, 5000, 100, 50);

	cvNamedWindow("main", CV_WINDOW_AUTOSIZE);
	IplImage * const img = cvCreateImage(cvSize(1024, 512), IPL_DEPTH_8U, 3);

	//cvSetMouseCallback("main", mouse_handler, job);
	cvShowImage("main", img);
	cvSetMouseCallback("main", mouse_handler, job);
	while (cvWaitKey(0) != 27)
		;
	
	vector_end(job);
	printer_footer(job);
	printer_disconnect(job);

	return 0;
}
