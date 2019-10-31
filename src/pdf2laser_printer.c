#include "pdf2laser_printer.h"
#include <arpa/inet.h>        // for inet_ntoa
#include <errno.h>            // for errno, EINTR, EAGAIN, EBADF, EIO
#include <netdb.h>            // for addrinfo, freeaddrinfo, getaddrinfo
#include <netinet/in.h>       // for sockaddr_in, ntohs
#include <stdint.h>           // for int32_t, uint32_t, uint8_t
#include <stdio.h>            // for perror, fprintf, NULL, fileno, printf, snprintf
#include <string.h>           // for strchr
#ifdef __linux
#include <sys/sendfile.h>     // for sendfile
#endif
#include <sys/socket.h>       // for connect, socket, PF_UNSPEC, SOCK_STREAM
#include <sys/stat.h>         // for fstat, stat
#include <unistd.h>           // for alarm, close, sleep, ssize_t

//bool debug = false;
char *queue = "";

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
bool printer_send(const char *host, FILE *pjl_file, const char *job_name)
{
	char local_hostname[1024];
	char *first_dot;

	gethostname(local_hostname, 1024);
	if ((first_dot = strchr(local_hostname, '.'))) {
		*first_dot = '\0';
	}

	uint8_t lpdres;
	int32_t p_sock = printer_connect(host, PRINTER_MAX_WAIT);

	write(p_sock, "\002\r\n", 3);
	read(p_sock, &lpdres, 1);
	if (lpdres) {
		fprintf (stderr, "Bad response from %s, %u\n", host, lpdres);
		return false;
	}

	struct stat file_stat;
	if (fstat(fileno(pjl_file), &file_stat)) {
		perror("Error reading pjl file\n");
		return false;
	}

	char job_header[10240];
	snprintf(job_header, 10240, "\003%u dfA%s%s\r\n", (uint32_t)file_stat.st_size, job_name, local_hostname);

	write(p_sock, job_header, strlen(job_header));
	read(p_sock, &lpdres, 1);
	if (lpdres) {
		fprintf(stderr, "Bad response from %s, %u\n", host, lpdres);
		return false;
	}

#ifdef __linux
	ssize_t bs = 0;
	size_t bytes_sent = 0;
	size_t count = file_stat.st_size;

	while (bytes_sent < count) {
		if ((bs = sendfile(p_sock, fileno(pjl_file), 0, count - bytes_sent)) <= 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			perror("sendfile filed");
			return -1;
		}
		bytes_sent += bs;
	}

	printf("Job size: %d (%d)\n", bytes_sent, count);
#else
	{
		char buffer[102400];
		size_t rc;
		while ((rc = fread(buffer, 1, 102400, pjl_file)) > 0)
			write(p_sock, buffer, rc);

		printf("Job size: %d\n", (int)file_stat.st_size);
	}
#endif


	return printer_disconnect(p_sock);
}
