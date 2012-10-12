/**
 * @file messenger.h
 * @ingroup messenger
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 6, 2012
 *
 * @brief File containing methods for sending and receiving messages between clients and server.
 */

#ifndef MESSENGER_H_
#define MESSENGER_H_

#include "structs.h"

void request_to_string(request_s *request, char *message);
void string_to_request(char *message, request_s *request);
void response_to_string(response_s *response, char *message);
void string_to_response(char *message, response_s *response);
void send_request_message(int server_fd, request_s * request);
void send_response_message(int client_fd, response_s *response);
void receive_response_message(int server_fd, response_s *response);
void send_receive_message(int server_fd, request_s *request, response_s *response);

#endif /* MESSENGER_H_ */
