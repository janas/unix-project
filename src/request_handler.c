/**
 * @file request_handler.c
 * @ingroup request_handler
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 12, 2012
 *
 * @brief File containing methods for handling requests received from clients.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "board_handler.h"
#include "lists.h"
#include "messenger.h"
#include "structs.h"
#include "thread_handler.h"

/**
 * Creates new player structure given client file descriptor and nick name.
 * @param[in]  client_fd  File descriptor of a client that is currently served.
 * @param[out] new_player Pointer to a structure containing player information.
 * @param[in]  nick       Nick name of newly created player.
 * @retval  0 Upon successful creation of new player.
 * @retval -1 If an error during memory allocation occurs.
 * \sa player_s
 */
int
create_new_player(int client_fd, player_s **new_player, char *nick) {
	(*new_player) = malloc(sizeof(player_s));
	if ((*new_player) == NULL) {
		fprintf(stderr, "Failed to allocate memory for new player\n");
		return -1;
	}
	(*new_player)->game_id = 0;
	(*new_player)->player_fd = client_fd;
	strncpy((*new_player)->player_nick, nick, MAX_NICK_LEN);
	return 0;
}

/**
 * Randomly chooses player index.
 * @return Index of a selected player. It can be 0 or 1.
 */
int
get_random_player() {
	int idx;
	unsigned int iseed = (unsigned int) time(NULL);
	srand(iseed);
	idx = (int) (rand() / (RAND_MAX + 1.0) * 2.0);
	return idx;
}

/**
 * Randomly chooses free game ID in range from 1 to 100.
 * @param[in] games_list Pointer to the head of the games list.
 * @return Randomly generated game ID or -1 upon error.
 * \sa games_list_s
 */
int
get_next_free_game_id(games_list_s *games_list) {
	int id, flag = 0;
	unsigned int iseed = (unsigned int) time(NULL);
	games_list_s *list = games_list;
	srand(iseed);
	id = (int) (rand() % 100) + 1;
	while (list != NULL && list->value != NULL) {
		if (list->value->id == id) {
			flag = 1;
			break;
		}
		list = list->next;
	}
	if (flag == 0) {
		return id;
	}
	return -1;
}

/**
 * Creates new game structure given player information and size of the board.
 * @param[in]  games_list Pointer to the head of the games list.
 * @param[out] new_game   Pointer to a structure containing game information.
 * @param[in]  player     Pointer to a structure containing player information.
 * @param[in]  size       Size of the board to create.
 * @retval  0 Upon successful creation of new game.
 * @retval -1 If an error during creation occurs.
 * \sa games_list_s game_s player_s
 */
int
create_new_game(games_list_s *games_list, game_s **new_game,
		player_s *player, int size) {
	int i, new_id = -1;
	(*new_game) = malloc(sizeof(game_s));
	if ((*new_game) == NULL) {
		fprintf(stderr, "Failed to allocate memory for new board\n");
		return -1;
	}
	while (new_id == -1) {
		new_id = get_next_free_game_id(games_list);
	};
	(*new_game)->id = new_id;
	(*new_game)->free = size * size;
	(*new_game)->current_player = -1;
	(*new_game)->no_connected_players = 0;
	(*new_game)->no_connected_spectators = 0;
	(*new_game)->board = create_new_board(size);
	(*new_game)->state = GAME_STATE_WAITING;
	(*new_game)->players[0] = player;
	(*new_game)->players[1] = NULL;
	for (i = 0; i < SPECTATORS_NO; i++) {
		(*new_game)->spectators[i] = -1;
	}

	return 0;
}

/**
 * Adds client file descriptor to a game spectators set.
 * @param[in] client_fd File descriptor of a client that is added to spectators.
 * @param[in] game      Pointer to a game structure that a spectator to be added.
 * \sa game_s
 */
void
update_spectators(int client_fd, game_s *game) {
	int i;
	for (i = 0; i < SPECTATORS_NO; i++) {
		if (game->spectators[i] != -1) {
			continue;
		} else {
			game->spectators[i] = client_fd;
			break;
		}
	}
}

/**
 * Clears main file descriptor set by removing all spectators' file descriptors.
 * @param[in] fds   Bit array holding file descriptor to be served by the server.
 * @param[in] tdata Pointer to a structure containing information about
 * \sa thread_data_s
 */
void
clear_spectators_fds(fd_set *fds, thread_data_s *tdata) {
	int i;
	for (i = 0; i < SPECTATORS_NO; i++) {
		if (tdata->spectators_fd[i] != -1) {
			if (FD_ISSET(tdata->spectators_fd[i], fds)) {
				FD_CLR(tdata->spectators_fd[i], fds);
			}
		}
	}
}

/**
 * Removes spectator from a given game.
 * @param[in] client_fd File descriptor of a client that is removed from spectators.
 * @param[in] game      Pointer to a game structure that a spectator to be removed.
 * \sa game_s
 */
void
set_spectator_fd_unused(int client_fd, game_s *game) {
	int i;
	for (i = 0; i < SPECTATORS_NO; i++) {
		if (game->spectators[i] == client_fd) {
			game->spectators[i] = -1;
		}
	}
}

/**
 * Handles game login request sent from a connecting client.
 * @param[in] client_fd    File descriptor of a client that is logged to server.
 * @param[in] request      Pointer to a structure containing request data.
 * @param[in] players_list Pointer to the head of the players list.
 * \sa request_s players_list_s
 */
void
handle_game_login_request(int client_fd, request_s *request,
		players_list_s *players_list) {
	char nick[MAX_NICK_LEN];
	player_s *player = NULL;
	response_s response;
	memset(response.payload, 0, MAX_RSP_SIZE);
	response.type = MSG_LOGIN_RSP;
	strncpy(nick, request->payload, MAX_NICK_LEN);
	if (find_player_by_nick(players_list, nick) == 0) {
		response.error = MSG_RSP_ERROR_NICK_EXISTS;
		send_response_message(client_fd, &response);
		return;
	}
	if (create_new_player(client_fd, &player, nick) == -1) {
		response.error = MSG_RSP_INTERNAL_SERVER_ERROR;
		send_response_message(client_fd, &response);
		return;
	}
	if (add_player_to_list(players_list, player) == -1) {
		response.error = MSG_RSP_INTERNAL_SERVER_ERROR;
		send_response_message(client_fd, &response);
		return;
	}
	response.error = MSG_RSP_ERROR_NONE;
	send_response_message(client_fd, &response);
}

/**
 * Handles client request to list all players connected to the server.
 * @param[in] client_fd    File descriptor of a client that is currently served.
 * @param[in] players_list Pointer to the head of the players list.
 * \sa players_list_s
 */
void
handle_players_list_request(int client_fd, players_list_s **players_list) {
	int i = 0, len = 0, count;
	char temp[MAX_NICK_LEN + 1];
	players_list_s *list = *players_list;
	response_s response;
	response.type = MSG_PLAYERS_LIST_RSP;
	count = (MAX_REQ_SIZE) / MAX_NICK_LEN;

	memset(response.payload, '0', MAX_RSP_SIZE);
	while (list != NULL && list->value != NULL && i <= count) {
		snprintf(temp, MAX_NICK_LEN + 1, "%s%s", list->value->player_nick,
				PAYLOAD_DELIM);
		strncpy(response.payload + len, temp, MAX_NICK_LEN);
		len += strlen(temp);
		list = list->next;
		i++;
	}
	response.error = MSG_RSP_ERROR_NONE;
	send_response_message(client_fd, &response);
}

/**
 * Handles client request to list all games that are currently on the server.
 * @param[in] client_fd  File descriptor of a client that is currently served.
 * @param[in] games_list Pointer to the head of the games list.
 * \sa games_list_s
 */
void
handle_game_list_request(int client_fd, games_list_s **games_list) {
	int i = 0, len = 0;
	char temp[128];
	games_list_s *list = *games_list;
	response_s response;
	response.type = MSG_GAMES_LIST_RSP;

	memset(response.payload, '0', MAX_RSP_SIZE);
	while (list != NULL && list->value != NULL) {
		if (list->value->players[1] == NULL) {
			snprintf(temp, 128, "%d%s%d%s%d%s%s%s", list->value->id,
					INNER_DELIM, get_board_size(list->value->board),
					INNER_DELIM, (SPECTATORS_NO - list->value->no_connected_spectators),
					INNER_DELIM, list->value->players[0]->player_nick,
					PAYLOAD_DELIM);
			strncpy(response.payload + len, temp, 128);
			len += strlen(temp);
		} else if (list->value->players[1] != NULL) {
			snprintf(temp, 128, "%d%s%d%s%d%s%s%s%s%s", list->value->id,
					INNER_DELIM, get_board_size(list->value->board),
					INNER_DELIM, (SPECTATORS_NO - list->value->no_connected_spectators),
					INNER_DELIM, list->value->players[0]->player_nick,
					INNER_DELIM, list->value->players[1]->player_nick,
					PAYLOAD_DELIM);
			strncpy(response.payload + len, temp, 128);
			len += strlen(temp);
		}
		list = list->next;
		i++;
	}

	response.error = MSG_RSP_ERROR_NONE;
	send_response_message(client_fd, &response);
}

/**
 * Handles client request to create new game.
 * @param[in] client_fd        File descriptor of a client that is currently served.
 * @param[in] request          Pointer to a structure containing request data.
 * @param[in] players_list     Pointer to the head of the players list.
 * @param[in] games_list       Pointer to the head of the games list.
 * @param[in] games_list_mutex Pointer to a mutex guarding games list.
 * \sa request_s players_list_s games_list_s pthread_mutex_t
 */
void
handle_create_new_game_request(int client_fd, request_s *request,
		players_list_s *players_list, games_list_s *games_list,
		pthread_mutex_t *games_list_mutex) {
	int ret;
	response_s response;
	game_s *game = NULL;
	player_s *player = NULL;
	int size = atoi(request->payload);
	response.type = MSG_CREATE_GAME_RSP;

	if (size < 4 || size > 20) {
		response.error = MSG_RSP_ERROR_WRONG_BORAD_SIZE;
		send_response_message(client_fd, &response);
		return;
	}
	get_player_by_file_desc(players_list, &player, client_fd);
	if (player == NULL) {
		response.error = MSG_RSP_INTERNAL_SERVER_ERROR;
		send_response_message(client_fd, &response);
		return;
	}
	if (create_new_game(games_list, &game, player, size) == -1) {
		response.error = MSG_RSP_INTERNAL_SERVER_ERROR;
		send_response_message(client_fd, &response);
		return;
	}

	pthread_mutex_lock(games_list_mutex);
	ret = add_game_to_list(games_list, game);
	pthread_mutex_unlock(games_list_mutex);

	if (ret == -1) {
		response.error = MSG_RSP_INTERNAL_SERVER_ERROR;
		send_response_message(client_fd, &response);
		return;
	}

	game->no_connected_players++;
	snprintf(response.payload, 4, "%d", game->id);
	response.error = MSG_RSP_ERROR_NONE;
	send_response_message(client_fd, &response);
}

/**
 * Handles client request to connect to existing game by initializing new thread that will
 * serve all communication between server and clients.
 * @param[in] client_fd          File descriptor of a client that is currently served.
 * @param[in] request            Pointer to a structure containing request data.
 * @param[in] base_rdfs          Bit array holding file descriptor to be served by the server.
 * @param[in] players_list       Pointer to a list holding players.
 * @param[in] games_list         Pointer to a list holding games.
 * @param[in] threads_list       Pointer to a list holding threads.
 * @param[in] players_list_mutex Pointer to a mutex guarding players list.
 * @param[in] games_list_mutex   Pointer to a mutex guarding games list.
 * @param[in] threads_list_mutex  Pointer to a mutex guarding threads list.
 * \sa request_s players_list_s games_list_s threads_list_s
 */
void
handle_connect_to_existing_game_request(int client_fd, request_s *request,
		fd_set *base_rdfs, players_list_s **players_list,
		games_list_s **games_list, threads_list_s **threads_list,
		pthread_mutex_t *players_list_mutex, pthread_mutex_t *games_list_mutex,
		pthread_mutex_t *threads_list_mutex) {
	int game_id = atoi(request->payload);
	thread_data_s data;
	response_s response;
	game_s *game = NULL;
	player_s *player = NULL;
	response.type = MSG_CONNECT_GAME_RSP;

	get_game_by_id(*games_list, &game, game_id);
	if (game == NULL) {
		response.error = MSG_RSP_ERROR_WRONG_GAME_ID;
		send_response_message(client_fd, &response);
		return;
	}
	if (game->no_connected_players >= 2) {
		response.error = MSG_RSP_ERROR_TOO_MANY_PLAYERS;
		send_response_message(client_fd, &response);
		return;
	}

	get_player_by_file_desc(*players_list, &player, client_fd);
	if (player == NULL) {
		response.error = MSG_RSP_INTERNAL_SERVER_ERROR;
		send_response_message(client_fd, &response);
		return;
	}
	game->no_connected_players++;
	game->players[1] = player;
	game->state = GAME_STATE_STARTED;
	data.games_list = games_list;
	data.players_list = players_list;
	data.parent_pid = getpid();
	data.game = game;
	data.players_list_mutex = players_list_mutex;
	data.games_list_mutex = games_list_mutex;
	data.threads_list_mutex = threads_list_mutex;
	data.threads_list = threads_list;
	data.game->current_player = game->players[get_random_player()]->player_fd;
	data.players_fd[0] = game->players[0]->player_fd;
	data.players_fd[1] = game->players[1]->player_fd;
	memcpy(data.spectators_fd, game->spectators, SPECTATORS_NO * sizeof(int));
	data.rd_fds = base_rdfs;
	FD_CLR(data.players_fd[0], base_rdfs);
	FD_CLR(data.players_fd[1], base_rdfs);
	clear_spectators_fds(base_rdfs, &data);
	initialize_thread(&data, *threads_list, threads_list_mutex);
	response.error = MSG_RSP_ERROR_NONE;
	send_response_message(client_fd, &response);
}

/**
 * Handles client request to connect as a spectator when a game is not started
 * yet (second player is not connected).
 * @param[in] client_fd    File descriptor of a client that is currently served.
 * @param[in] request      Pointer to a structure containing request data.
 * @param[in] base_rdfs    Bit array holding file descriptor to be served by the server.
 * @param[in] games_list   Pointer to a list holding games.
 * @param[in] threads_list Pointer to a list holding threads.
 * \sa request_s games_list_s threads_list_s
 */
void
handle_connect_as_spectator_request(int client_fd, request_s *request,
		fd_set *base_rdfs, games_list_s *games_list,
		threads_list_s *threads_list) {
	int game_id = atoi(request->payload);
	response_s response;
	game_s *game = NULL;
	thread_s *thread = NULL;
	response.type = MSG_CONNECT_SPECTATOR_RSP;
	get_game_by_id(games_list, &game, game_id);
	if (game == NULL) {
		response.error = MSG_RSP_ERROR_WRONG_GAME_ID;
		send_response_message(client_fd, &response);
		return;
	}
	if (game->no_connected_spectators >= 5) {
		response.error = MSG_RSP_ERROR_TOO_MANY_SPECTATORS;
		send_response_message(client_fd, &response);
		return;
	}

	get_thread_by_id(threads_list, &thread, game_id);
	if (thread == NULL) {
		update_spectators(client_fd, game);
		game->no_connected_spectators++;
		response.error = MSG_RSP_ERROR_NONE;
		send_response_message(client_fd, &response);
		printf("New spectator connected\n");
		return;
	}
	update_spectators(client_fd, game);
	game->no_connected_spectators++;
	FD_CLR(client_fd, base_rdfs);
	response.error = MSG_RSP_ERROR_NONE;
	send_response_message(client_fd, &response);
	pthread_kill(thread->pthread, SIGRTMIN + 1);
}

/**
 * Handles client (spectator) request to back to main menu before
 * new thread is started (second player connects).
 * @param[in] client_fd  File descriptor of a client that is currently served.
 * @param[in] request    Pointer to a structure containing request data.
 * @param[in] games_list Pointer to a list holding games.
 * \sa request_s games_list_s
 */
void
handle_back_to_menu_request(int client_fd, request_s *request,
		games_list_s *games_list) {
	int game_id = atoi(request->payload);
	response_s response;
	game_s *game = NULL;
	response.type = MSG_BACK_TO_MENU_RSP;
	get_game_by_id(games_list, &game, game_id);
	if (game == NULL) {
		response.error = MSG_RSP_ERROR_WRONG_GAME_ID;
		send_response_message(client_fd, &response);
		return;
	}
	game->no_connected_spectators--;
	set_spectator_fd_unused(client_fd, game);
	response.error = MSG_RSP_ERROR_NONE;
	send_response_message(client_fd, &response);
	printf("Spectator disconnected\n");
}

/**
 * Handles client request to serve in game menu when the other player is not connected yet.
 * It simply prints a message saying to wait for the opponent.
 * @param[in] client_fd File descriptor of a client that is currently served.
 * @param[in] request   Pointer to a structure containing request data.
 * \sa request_s
 */
void
handle_game_message(int client_fd, request_s *request) {
	response_s response;
	response.type = request->type + 1;
	response.error = MSG_RSP_ERROR_WAIT_OPPONENT;
	send_response_message(client_fd, &response);
}

/**
 * Handles client request to leave a game before new thread is
 * started (second player connects).
 * @param[in] client_fd        File descriptor of a client that is currently served.
 * @param[in] request          Pointer to a structure containing request data.
 * @param[in] games_list       Pointer to a list holding games.
 * @param[in] games_list_mutex Pointer to a mutex guarding games list.
 * \sa request_s games_list_s
 */
void
handle_leave_game_request(int client_fd, request_s *request,
		games_list_s **games_list, pthread_mutex_t *games_list_mutex) {
	int game_id;
	response_s response;
	game_s *game = NULL;
	response.type = MSG_LEAVE_RSP;
	game_id = atoi(request->payload);
	get_game_by_id(*games_list, &game, game_id);
	if (game == NULL) {
		response.error = MSG_RSP_ERROR_WRONG_GAME_ID;
		send_response_message(client_fd, &response);
		return;
	}
	response.error = MSG_RSP_ERROR_NONE;
	send_response_message(client_fd, &response);
	pthread_mutex_lock(games_list_mutex);
	remove_game_from_list(games_list, game);
	pthread_mutex_unlock(games_list_mutex);
}
