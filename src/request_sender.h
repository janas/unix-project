/**
 * @file request_sender.h
 * @ingroup request_sender
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 12, 2012
 *
 * @brief File containing methods for sending requests to the server.
 */

#ifndef REQUEST_SENDER_H_
#define REQUEST_SENDER_H_

void print_error_message(message_error_e error);
void send_game_login_request(int server_fd, player_mode_e *mode);
void send_players_list_request(int server_fd);
void send_games_list_request(int server_fd);
void send_create_game_request(int server_fd, player_mode_e *mode, int *game_id);
void send_connect_game_request(int server_fd, player_mode_e *mode, int *game_id);
void send_connect_spectator_request(int server_fd, player_mode_e *mode, int *game_id);
void send_print_board_request(int server_fd);
void send_check_turn_request(int server_fd);
void send_make_move_request(int server_fd, player_mode_e *mode);
void send_leave_message_request(int server_fd);
void send_giveup_request(int server_fd, player_mode_e *mode, int *game_id);
void send_back_to_menu_request(int server_fd, player_mode_e *mode, int *game_id);
void print_spectator_board(response_s *response);

#endif /* REQUEST_SENDER_H_ */
