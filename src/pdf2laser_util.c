#include "pdf2laser_util.h"
#include <errno.h>         // for errno, EAGAIN, EINTR
#include <inttypes.h>      // for PRId64, PRIu64, int64_t
#include <stddef.h>        // for size_t, NULL
#include <stdio.h>         // for perror, printf
#ifdef __linux
#include <sys/sendfile.h>  // for sendfile
#endif
#include <sys/stat.h>      // for fstat, stat
#include <unistd.h>        // for ssize_t

int pdf2laser_sendfile(int out_fd, int in_fd)
{
	struct stat file_stat;
	if (fstat(in_fd, &file_stat)) {
		perror("Error stating file");
          return -1;
	}

#ifdef __linux
	ssize_t bs = 0;
	size_t bytes_sent = 0;
	size_t count = file_stat.st_size;

	while (bytes_sent < count) {
		if ((bs = sendfile(out_fd, in_fd, NULL, count - bytes_sent)) <= 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			perror("sendfile failed");
			return errno;
		}
		bytes_sent += bs;
	}

	printf("Job size: %"PRIu64"(%"PRId64")\n", bytes_sent, count);
#else
	char buffer[102400];
	size_t rc;
	while ((rc = read(in_fd, buffer, 102400)) > 0)
		write(out_fd, buffer, rc);

	printf("Job size: %"PRId64"\n", (int64_t)file_stat.st_size);
#endif

	return 0;
}
