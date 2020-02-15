#include "pdf2laser_util.h"
#include <errno.h>
#include <stddef.h>        // for ssize_t, size_t
#include <inttypes.h>      // for PRId64
#include <sys/stat.h>      // for fstat, stat
#ifdef __linux
#include <sys/sendfile.h>  // for sendfile
#endif
#include <stdio.h>         // for printf
//#include <fcntl.h>
#include <unistd.h>

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

	printf("Job size: %d\n", (int)file_stat.st_size);
#endif

	return 0;
}
