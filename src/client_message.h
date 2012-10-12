/**
 * @file client_message.h
 * @ingroup client_message
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Aug 30, 2012
 *
 * @brief File containing methods responsible for displaying incoming messages (without sending a request).
 */

#ifndef CLIENT_MESSAGE_H_
#define CLIENT_MESSAGE_H_

#include "structs.h"

void get_print_board_message(response_s *response);
void get_message_from_opponent(response_s *response);
void get_cleanup_message(response_s *response, player_mode_e *mode);
void get_print_result_message(response_s *response, player_mode_e *mode);
void get_print_lost_message(response_s *response, player_mode_e *mode);
void get_print_draw_message(response_s *response, player_mode_e *mode);

#endif /* CLIENT_MESSAGE_H_ */
