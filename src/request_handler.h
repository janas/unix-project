/**
 * @file request_handler.h
 * @ingroup request_handler
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 12, 2012
 *
 * @brief File containing methods for handling requests received from clients.
 */

#ifndef REQUEST_HANDLER_H_
#define REQUEST_HANDLER_H_

void handle_game_login_request(int client_fd, request_s *request,
		players_list_s *players_list);
void handle_players_list_request(int client_fd, players_list_s **players_list);
void handle_game_list_request(int client_fd, games_list_s **games_list);
void handle_create_new_game_request(int client_fd, request_s *request,
		players_list_s *players_list, games_list_s *games_list,
		pthread_mutex_t *games_list_mutex);
void handle_connect_to_existing_game_request(int client_fd, request_s *request,
		fd_set *base_rdfs, players_list_s **players_list,
		games_list_s **games_list, threads_list_s **threads_list,
		pthread_mutex_t *players_list_mutex, pthread_mutex_t *games_list_mutex,
		pthread_mutex_t *threads_list_mutex);
void handle_connect_as_spectator_request(int client_fd, request_s *request,
		fd_set *base_rdfs, games_list_s *games_list,
		threads_list_s *threads_list);
void handle_back_to_menu_request(int client_fd, request_s *request,
		games_list_s *games_list);
void handle_game_message(int client_fd, request_s *request);
void handle_leave_game_request(int client_fd, request_s *request,
		games_list_s **games_list, pthread_mutex_t *games_list_mutex);

#endif /* REQUEST_HANDLER_H_ */
