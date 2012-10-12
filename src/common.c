/**
 * @file common.c
 * @ingroup common
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 6, 2012
 *
 * @brief File containing methods that are commonly used in server and client code.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

#include "config.h"

/**
 * Registers given function to handle a specified signal upon arrival.
 * @param[in] f     Pointer to a function responsible for handling a signal.
 * @param[in] sigNo The number of a signal.
 * @retval  0  Upon successful registering function and signal.
 * @retval -1 Upon error.
 */
int
sethandler(void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

/**
 * Makes a new socket based on a domain and a type.
 * @param[in] domain The type of the communication domain.
 * @param[in] type   The type of the communication semantics.
 * @return File descriptor of a created socket.
 */
int
make_socket(int domain, int type) {
	int sock;
	sock = socket(domain, type, 0);
	if (sock < 0)
		ERR("socket");
	return sock;
}

/**
 * Writes content of a specified file descriptor to a buffer.
 * It also checks the size of the writing buffer.
 * @param[in]  fd    Number of file descriptor to read from.
 * @param[out] buf   Buffer to which write from a specified file descriptor.
 * @param[in]  count Size of the buffer to write.
 * @return The number of bytes read.
 */
ssize_t
bulk_read(int fd, char *buf, size_t count) {
	int c;
	size_t len = 0;
	do {
		c = TEMP_FAILURE_RETRY(read(fd, buf, count));
		if (c < 0)
			return c;
		if (0 == c)
			return len;
		buf += c;
		len += c;
		count -= c;
	} while (count > 0);
	return len;
}

/**
 * Writes content of a specified buffer to a file descriptor given as a number.
 * It also checks the size of the writing buffer.
 * @param[in] fd    Number of file descriptor to write to.
 * @param[in] buf   Buffer from which write to a specified file descriptor.
 * @param[in] count Size of the buffer to write.
 * @return The number of bytes written.
 */
ssize_t
bulk_write(int fd, char *buf, size_t count) {
	int c;
	size_t len = 0;
	do {
		c = TEMP_FAILURE_RETRY(write(fd, buf, count));
		if (c < 0)
			return c;
		buf += c;
		len += c;
		count -= c;
	} while (count > 0);
	return len;
}

/**
 * Reads characters from a standard input one by one.
 * Terminates if Return is pressed or number of desired characters are read.
 * Then clears the stdin buffer.
 * @param[out] buffer Buffer to store read out characters.
 * @param[in]  size   Number of characters to read from standard input.
 */
void
read_line(char *buffer, int size) {
	int i = 0;
	char c;
	memset(buffer, 0, size);
	while (1) {
		c = fgetc(stdin);
		if (c == '\n' || c == EOF) {
			buffer[i] = '\0';
			return;
		}
		if (i >= size - 1) {
			buffer[i] = '\0';
			break;
		}
		buffer[i] = c;
		i++;
	}
	while (c != '\n' && c != EOF)
		c = fgetc(stdin);
}

/**
 * Computes the maximum number from those given as parameters.
 * @param[in] one First value to compare.
 * @param[in] two Second value to compare.
 * @return The maximum of the values of parameters.
 */
int
max(int one, int two) {
	if (one > two) {
		return one;
	} else if (two > one) {
		return two;
	}
	return one;
}
