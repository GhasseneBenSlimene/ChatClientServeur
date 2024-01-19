#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <poll.h>

#include "../include/common.h"

void test_error(bool test, char *str)
{
	if (test)
	{
		perror(str);
		exit(EXIT_FAILURE);
	}
}

size_t read_full(int fd, void *buf, size_t count)
{
	size_t total_received = 0;
	size_t bytes_received;
	char *buf_ptr = (char *)buf;

	while (total_received < count)
	{
		bytes_received = read(fd, buf_ptr + total_received, count - total_received);
		test_error(bytes_received <= 0, "read");
		total_received += bytes_received;

		if (bytes_received == 0)
			return 0;
	}
	return total_received;
}

size_t write_full(int fd, const void *buf, size_t count)
{
	size_t total_sent = 0;
	size_t bytes_sent;
	const char *buf_ptr = (const char *)buf;

	while (total_sent < count)
	{
		bytes_sent = write(fd, buf_ptr + total_sent, count - total_sent);
		test_error(bytes_sent <= 0, "write");
		total_sent += bytes_sent;
	}

	return total_sent;
}