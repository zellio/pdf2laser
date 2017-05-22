#include "pdf2laser_printer.h"

bool debug = false;
char *queue = NULL;

/**
 * Connect to a printer.
 *
 * @param host The hostname or IP address of the printer to connect to.
 * @param timeout The number of seconds to wait before timing out on the
 * connect operation.
 * @return A socket descriptor to the printer.
 */
static int32_t printer_connect(const char *host, const uint32_t timeout)
{
	int32_t socket_descriptor = -1;

	uint32_t i = 0;
	for (; i < timeout; i++) {
		struct addrinfo *res;
		struct addrinfo *addr;
		struct addrinfo base = { 0, PF_UNSPEC, SOCK_STREAM, 0, 0, NULL, NULL, NULL };

		int32_t error_code = getaddrinfo(host, "printer", &base, &res);

		/* Set an alarm to go off if the program has gone out to lunch. */
		alarm(SECONDS_PER_MIN);

		/* if getaddrinfo did not return an error code then we attempt to
		 * connect to the printer and establish a socket.
		 */
		if (!error_code) {
			for (addr = res; addr; addr = addr->ai_next) {
				const struct sockaddr_in * addr_in = (void*) addr->ai_addr;
				if (addr_in)
					printf("trying to connect to %s:%d\n",
						   inet_ntoa(addr_in->sin_addr),
						   ntohs(addr_in->sin_port));

				socket_descriptor = socket(addr->ai_family, addr->ai_socktype,
										   addr->ai_protocol);
				if (socket_descriptor >= 0) {
					if (!connect(socket_descriptor, addr->ai_addr,
								 addr->ai_addrlen)) {
						break;
					} else {
						close(socket_descriptor);
						socket_descriptor = -1;
					}
				}
			}
			freeaddrinfo(res);
		}

		if (socket_descriptor >= 0)
			break;

		/* Sleep for a second then try again. */
		sleep(1);
	}

	if (i >= timeout) {
		fprintf(stderr, "Cannot connect to %s\n", host);
		return -1;
	}

	/* Disable the timeout alarm. */
	alarm(0);

	/* Return the newly opened socket descriptor */
	return socket_descriptor;
}

/**
 * Disconnect from a printer.
 *
 * @param socket_descriptor the descriptor of the printer that is to be
 * disconnected from.
 * @return True if the printer connection was successfully closed, false otherwise.
 */
static bool printer_disconnect(int32_t socket_descriptor)
{
	int32_t error_code;
	error_code = close(socket_descriptor);

	/* Report on possible errors to standard error. */
	if (error_code == -1) {
		switch (errno) {
		case EBADF:
			perror("Socket descriptor given was not valid.");
			break;
		case EINTR:
			perror("Closing socket descriptor was interrupted by a signal.");
			break;
		case EIO:
			perror("I/O error occurred during closing of socket descriptor.");
			break;
		}
	}

	/* Return status of disconnect operation to the calling function. */
	if (error_code) {
		return false;
	} else {
		return true;
	}
}

/**
 *
 */
bool printer_send(const char *host, FILE *pjl_file)
{
	char localhost[HOSTNAME_NCHARS] = "";
	uint8_t lpdres;
	uint32_t socket_descriptor = -1;

	gethostname(localhost, sizeof(localhost));

	{
		char *first_dot = strchr(localhost, '.');
		if (first_dot) {
			*first_dot = '\0';
		}
	}

	if (debug)
		printf("printer host: '%s'\n", host);

	/* Connect to the printer. */
	socket_descriptor = printer_connect(host, PRINTER_MAX_WAIT);

	if (debug)
		printf("printer host: '%s' fd %d\n", host, socket_descriptor);

	{
		uint32_t buffer_size = 102400;
		char* buffer[buffer_size] = { '\0' };
		char* ptr = buffer;
		int32_t write_count = 0;

		// talk to printer
		write_count = snprintf(buffer_ptr, buffer_size, "\002%s\n", queue);

		write(socket_descriptor, buffer, strlen(buffer));
		read(socket_descriptor, &lpdres, 1);
		if (lpdres) {
			fprintf (stderr, "Bad response from %s, %u\n", host, lpdres);
			return false;
		}
	}

	{
		uint32_t buffer_size = 102400;
		char* buffer[buffer_size] = { '\0' };
		char* ptr = buffer;
		int32_t wc = 0;

		wc = snprintf(buffer_ptr, buffer_size, "H%s\n", localhost);
		buffer_size -= wc;
		buffer_ptr += wc;

		wc = snprintf(buffer_ptr, buffer_size, "P%s\n", job_user);
		buffer_size -= wc;
		buffer_ptr += wc;

		snprintf(buffer_ptr, buffer_size, "J%s\n", job_title);
		buffer_size -= wc;
		buffer_ptr += wc;

		sprintf(buffer_ptr, buffer_size, "ldfA%s%s\n", job_name, localhost);
		buffer_size -= wc;
		buffer_ptr += wc;

		sprintf(buffer_ptr, buffer_size, "UdfA%s%s\n", job_name, localhost);
		buffer_size -= wc;
		buffer_ptr += wc;

		sprintf(buffer_ptr, buffer_size, "N%s\n", job_title);
		buffer_size -= wc;
		buffer_ptr += wc;

		sprintf(buffer_ptr, buffer_size, "\002%d cfA%s%s\n", (int32_t)(102400 - buffer_size), job_name, localhost);
		buffer_size -= wc;
		buffer_ptr += wc;

		write(socket_descriptor, buf + strlen(buf) + 1, strlen(buf + strlen(buf) + 1));

		read(socket_descriptor, &lpdres, 1);
		if (lpdres) {
			fprintf(stderr, "Bad response from %s, %u\n", host, lpdres);
			return false;
		}

		write(socket_descriptor, (char *)buf, strlen(buf) + 1);
		read(socket_descriptor, &lpdres, 1);

		if (lpdres) {
			fprintf(stderr, "Bad response from %s, %u\n", host, lpdres);
			return false;
		}
	}

/* 	{ */
/* 		{ */
/* 			struct stat file_stat; */
/* 			if (fstat(fileno(pjl_file), &file_stat)) { */
/* 				perror(buf); */
/* 				return false; */
/* 			} */
/* 			sprintf((char*)buf, "\003%u dfA%s%s\n", (int)file_stat.st_size, job_name, localhost); */
/* 			printf("job '%s': size %u\n", job_name, (int)file_stat.st_size); */
/* 		} */

/* 		write(socket_descriptor, (char *)buf, strlen(buf)); */
/* 		read(socket_descriptor, &lpdres, 1); */

/* 		if (lpdres) { */
/* 			fprintf(stderr, "Bad response from %s, %u\n", host, lpdres); */
/* 			return false; */
/* 		} */

/* 		{ */
/* 			int l; */
/* 			while ((l = fread((char*)buf, 1, sizeof (buf), pjl_file)) > 0) { */
/* 				write(socket_descriptor, buf, l); */
/* 			} */
/* 		} */
/* 	} */

/* 	// dont wait for a response... */
/* 	printer_disconnect(socket_descriptor); */

/* 	return true; */
}
