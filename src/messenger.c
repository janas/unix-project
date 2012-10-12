/**
 * @file messenger.c
 * @ingroup messenger
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 6, 2012
 *
 * @brief File containing methods for sending and receiving messages between clients and server.
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
#include "common.h"
#include "messenger.h"
#include "structs.h"

/**
 * Converts request structure into a character string.
 * @param[in]  request Pointer to a structure containing request data.
 * @param[out] message Character string containing converted request data.
 * \sa request_s
 */
void
request_to_string(request_s *request, char *message) {
	int len;
	memset(message, 0, MAX_MSG_SIZE);
	len = strlen(request->payload);
	if (len > 0) {
		snprintf(message, MAX_MSG_SIZE, "%d%s%s", request->type, MSG_DELIM,
				request->payload);
	} else {
		snprintf(message, MAX_MSG_SIZE, "%d%s", request->type, MSG_DELIM);
	}
}

/**
 * Converts request string into a request structure.
 * @param[in]  message Character string containing request data.
 * @param[out] request Pointer to a structure containing converted information.
 * \sa request_s
 */
void
string_to_request(char *message, request_s *request) {
	char delims[] = MSG_DELIM;
	char *result = NULL;
	result = strtok(message, delims);
	if (result != NULL) {
		request->type = atoi(result);
	}
	result = strtok(NULL, delims);
	if (result != NULL) {
		strncpy(request->payload, result, MAX_REQ_SIZE);
	}
}

/**
 * Converts response structure into a character string.
 * @param[in]  response Pointer to a structure containing response data.
 * @param[out] message  Character string containing converted response data.
 * \sa response_s
 */
void
response_to_string(response_s *response, char *message) {
	int len = strlen(response->payload);
	memset(message, 0, MAX_MSG_SIZE);
	if (len > 0) {
		snprintf(message, MAX_MSG_SIZE, "%d%s%d%s%s", response->type, MSG_DELIM,
				response->error, MSG_DELIM, response->payload);
	} else {
		snprintf(message, MAX_MSG_SIZE, "%d%s%d%s", response->type, MSG_DELIM,
				response->error, MSG_DELIM);
	}
}

/**
 * Converts response string into a response structure.
 * @param[in]  message  Character string containing response data.
 * @param[out] response Pointer to a structure containing converted information.
 * \sa response_s
 */
void
string_to_response(char *message, response_s *response) {
	char delims[] = MSG_DELIM;
	char *result = NULL;
	result = strtok(message, delims);
	if (result != NULL) {
		response->type = atoi(result);
	}
	result = strtok(NULL, delims);
	if (result != NULL) {
		response->error = atoi(result);
	}
	result = strtok(NULL, delims);
	if (result != NULL) {
		strncpy(response->payload, result, MAX_REQ_SIZE);
	}
}

/**
 * Sends request of a client to the server.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 * @param[in] request   Pointer to a structure containing request data to be sent.
 * \sa request_s
 */
void
send_request_message(int server_fd, request_s * request) {
	ssize_t size;
	char *message = malloc(MAX_MSG_SIZE);
	if (message == NULL) {
		fprintf(stderr, "Unable to allocate memory for request string\n");
	}

	request_to_string(request, message);
	size = bulk_write(server_fd, message, MAX_MSG_SIZE);
	if (size == MAX_MSG_SIZE) {
		/* fprintf(stderr, "Request successfully sent to server fd %d\n", server_fd); */
	} else {
		fprintf(stderr, "Error!\n");
	}

	free(message);
}

/**
 * Sends response of a server to the client.
 * @param[in] client_fd  File descriptor of the socket connected to the client.
 * @param[in] response   Pointer to a structure containing response data to be sent.
 * \sa response_s
 */
void
send_response_message(int client_fd, response_s *response) {
	ssize_t size;
	char *message = malloc(MAX_MSG_SIZE);
	if (message == NULL) {
		fprintf(stderr, "Unable to allocate memory for response string\n");
	}

	response_to_string(response, message);
	size = bulk_write(client_fd, message, MAX_MSG_SIZE);
	if (size == MAX_MSG_SIZE) {
		fprintf(stderr, "Response successfully sent to client fd %d\n",	client_fd);
	} else {
		fprintf(stderr, "Error!\n");
	}

	free(message);
}

/**
 * Receives response message from the server.
 * @param[in]  server_fd File descriptor of the socket connected to the server.
 * @param[out] response  Pointer to a structure containing response data to which write.
 * \sa response_s
 */
void
receive_response_message(int server_fd, response_s *response) {
	ssize_t size;
	char *message = malloc(MAX_MSG_SIZE);
	if (message == NULL) {
		fprintf(stderr, "Unable to allocate memory for response string\n");
	}

	size = bulk_read(server_fd, message, MAX_MSG_SIZE);
	if (size == MAX_MSG_SIZE) {
		string_to_response(message, response);
	} else if (size <= 0) {
		fprintf(stderr, "\nError while reading from server\n");
		free(message);
		response = NULL;
		exit(EXIT_FAILURE);
	}

	free(message);
}

/**
 * Sends a request and receives a response message from the server.
 * @param[in]  server_fd File descriptor of the socket connected to the server.
 * @param[in]  request   Pointer to a structure containing response data to be sent.
 * @param[out] response  Pointer to a structure containing response data to which write.
 * \sa request_s response_s
 */
void
send_receive_message(int server_fd, request_s *request, response_s *response) {
	send_request_message(server_fd, request);
	receive_response_message(server_fd, response);
}
