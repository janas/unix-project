/**
 * @file common.h
 * @ingroup common
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 6, 2012
 *
 * @brief File containing methods that are commonly used in server and client code.
 */

#ifndef COMMON_H_
#define COMMON_H_

int sethandler( void (*f)(int), int sigNo);
int make_socket(int domain, int type);
ssize_t bulk_read(int fd, char *buf, size_t count);
ssize_t bulk_write(int fd, char *buf, size_t count);
void read_line(char *buffer, int size);
int max(int one, int two);

#endif /* COMMON_H_ */
