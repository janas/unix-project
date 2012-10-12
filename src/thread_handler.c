/**
 * @file thread_handler.c
 * @ingroup thread_handler
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Aug 14, 2012
 *
 * @brief File containing methods for handling requests
 * received from clients (during the game).
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "board_handler.h"
#include "config.h"
#include "common.h"
#include "lists.h"
#include "messenger.h"
#include "structs.h"

/**
 * A variable responsible for setting the value determining the main program loop termination.
 */
__thread volatile sig_atomic_t thwork = 1;

/**
 * A variable responsible for
 */
__thread volatile sig_atomic_t play = 1;

/**
 * A structure holding data/arguments used by the current thread.
 */
__thread thread_data_s tdata;

/**
 * A pointer to a bit array holding file descriptors being served by the current thread.
 */
__thread fd_set *global_thread_rdfs = NULL;

/**
 * A pointer to a variable holding the maximum file descriptor value that is in the set.
 */
__thread int *global_thrad_fdmax = NULL;

/**
 * Updates array of connected spectators when new spectator is connected.
 */
void
update_connected_spectators(void) {
	int i;
	pthread_t tid;
	games_list_s *list = *tdata.games_list;
	game_s *game = NULL;
	tid = pthread_self();
	if (list == NULL) {
		fprintf(stderr, "(Thread %d) Error while updating data\n", (int) tid);
		return;
	}
	get_game_by_id(list, &game, tdata.game->id);
	if (game == NULL) {
		fprintf(stderr, "(Thread %d) Error game list is null\n", (int) tid);
		return;
	}
	memcpy(tdata.spectators_fd, game->spectators, SPECTATORS_NO * sizeof(int));
	for (i = 0; i < SPECTATORS_NO; i++) {
		if (tdata.spectators_fd[i] != -1) {
			if (!FD_ISSET(tdata.spectators_fd[i], global_thread_rdfs)) {
				FD_SET(tdata.spectators_fd[i], global_thread_rdfs);
				*global_thrad_fdmax = max(*global_thrad_fdmax,
						tdata.spectators_fd[i]);
			}
		}
	}
}

/**
 * Sets given client's file descriptor as unused i.e. sets it
 * to -1. Then stops main thread loop by setting thwork to 0.
 * @param[in] client_fd File descriptor of a client that is currently served.
 */
void
update_connected_players(int client_fd) {
	if (tdata.players_fd[0] == client_fd) {
		tdata.players_fd[0] = -1;
	} else if (tdata.players_fd[1] == client_fd) {
		tdata.players_fd[1] = -1;
	}
	thwork = 0;
	/* tdata.game->no_connected_players--; */
}

/**
 * Serves SIGRTMIN + 1 by calling update_connected_spectators.
 * @param[in] sig A signal number that will be served.
 * \sa update_connected_players
 */
void
sig_handler(int sig) {
	sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGRTMIN + 1);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	printf("(Thread %d) New spectator connected\n", (int) pthread_self());
	update_connected_spectators();
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

/**
 * Cleans up current thread and removes it from the list.
 */
void
cleanup_handler(void *arg) {
	int i, j;
	response_s response;
	games_list_s **list = tdata.games_list;
	thread_s *thread = NULL;
	threads_list_s **tlist = tdata.threads_list;
	response.type = MSG_CLEANUP_RSP;
	response.error = MSG_RSP_ERROR_NONE;
	printf("Thread %d cleanup handler goes\n", (int) pthread_self());
	for (i = 0; i < SPECTATORS_NO; i++) {
		if (tdata.spectators_fd[i] != -1) {
			FD_SET(tdata.spectators_fd[i], tdata.rd_fds);
			if (play == 1) {
				send_response_message(tdata.spectators_fd[i], &response);
			}
		}
	}
	for (j = 0; j < 2; j++) {
		if (tdata.players_fd[j] != -1) {
			FD_SET(tdata.players_fd[j], tdata.rd_fds);
			if (play == 1) {
				send_response_message(tdata.players_fd[j], &response);
			}
		}
	}
	get_thread_by_id(*tlist, &thread, tdata.game->id);
	if (thread != NULL) {
		pthread_mutex_lock(tdata.threads_list_mutex);
		remove_thread_from_list(tlist, thread);
		pthread_mutex_unlock(tdata.threads_list_mutex);
	}
	destroy_board(tdata.game->board);
	pthread_mutex_lock(tdata.games_list_mutex);
	remove_game_from_list(list, tdata.game);
	pthread_mutex_unlock(tdata.games_list_mutex);
	kill(tdata.parent_pid, SIGRTMIN + 11);
}

/**
 * Checks if a player with a given file descriptor has his/her turn or it is an opponent's turn.
 * @param[in] client_fd File descriptor of a client that is currently served.
 * @return Current player's file descriptor or -1 when it is opponent's turn.
 */
int
check_current_player(int client_fd) {
	if (client_fd == tdata.game->players[0]->player_fd) {
		return tdata.game->players[1]->player_fd;
	} else if (client_fd == tdata.game->players[1]->player_fd) {
		return tdata.game->players[0]->player_fd;
	}
	return -1;
}

/**
 * Gets player's pawn given a file descriptor.
 * @param[in]  client_fd File descriptor of a client that is currently served.
 * @param[in]  game      Pointer to a game structure that is currently played.
 * @param[out] pawn      Character representing player's pawn on the board.
 * \sa game_s
 */
void
get_pawn(int client_fd, game_s *game, char *pawn) {
	if (client_fd == game->players[0]->player_fd) {
		*pawn = 'x';
	} else if (client_fd == game->players[1]->player_fd) {
		*pawn = 'o';
	}
}

/**
 * Sends a message with current state of the board to all connected spectators.
 */
void
send_broadcast_message(void) {
	int i, j, k, size, index = 0;
	char temp[NROWS * NCOLS];
	response_s response;
	response.type = MSG_PRINT_BOARD_SPC_RSP;
	size = get_board_size(tdata.game->board);
	if (size == -1) {
		response.error = MSG_RSP_INTERNAL_SERVER_ERROR;
		for (k = 0; k < SPECTATORS_NO; k++) {
			if (tdata.spectators_fd[k] == -1) {
				continue;
			}
			send_response_message(tdata.spectators_fd[k], &response);
		}
		return;
	}

	for (i = 0; i < NROWS; i++) {
		for (j = 0; j < NCOLS; j++) {
			temp[index] = tdata.game->board[i][j];
			index++;
		}
	}

	snprintf(response.payload, MAX_RSP_SIZE, "%d%s%s%s", size, PAYLOAD_DELIM,
			temp, PAYLOAD_DELIM);
	response.error = MSG_RSP_ERROR_NONE;

	for (k = 0; k < SPECTATORS_NO; k++) {
		if (tdata.spectators_fd[k] == -1) {
			continue;
		}
		send_response_message(tdata.spectators_fd[k], &response);
	}
}

/**
 * Sends a message to two players and all connected spectators saying that there
 * is a draw. Then the game is ended.
 */
void send_broadcast_draw_message(void) {
	int k;
	response_s response;
	response.type = MSG_PRINT_DRAW_RSP;
	response.error = MSG_RSP_ERROR_NONE;
	for (k = 0; k < SPECTATORS_NO; k++) {
		if (tdata.spectators_fd[k] == -1) {
			continue;
		}
		send_response_message(tdata.spectators_fd[k], &response);
	}
	send_response_message(tdata.game->players[0]->player_fd, &response);
	send_response_message(tdata.game->players[1]->player_fd, &response);
}

/**
 * Sends a message to all connected spectators saying who won the current game.
 * @param[in] client_fd File descriptor of a client that is currently served.
 */
void
send_broadcast_win_message(int client_fd) {
	int k, i = -1;
	response_s response;
	response.type = MSG_PRINT_RESULT_SPC_RSP;

	if (client_fd == tdata.game->players[0]->player_fd) {
		i = 0;
	} else if (client_fd == tdata.game->players[1]->player_fd) {
		i = 1;
	}
	if (i != -1) {
		snprintf(response.payload, MAX_RSP_SIZE, "%s%s%s", "Player ",
				tdata.game->players[i]->player_nick, " won the game!");
		response.error = MSG_RSP_ERROR_NONE;
		for (k = 0; k < SPECTATORS_NO; k++) {
			if (tdata.spectators_fd[k] == -1) {
				continue;
			}
			send_response_message(tdata.spectators_fd[k], &response);
		}
	}
}

/**
 * Handles a request to print out current state of the board.
 * @param[in] client_fd File descriptor of a client that is currently served.
 * @param[in] game      Pointer to a game structure that is currently played.
 * \sa game_s
 */
void
thread_handle_print_board_request(int client_fd, game_s *game) {
	int i, j, size, index = 0;
	char temp[NROWS * NCOLS];
	response_s response;
	response.type = MSG_PRINT_BOARD_RSP;
	size = get_board_size(game->board);
	if (size == -1) {
		response.error = MSG_RSP_INTERNAL_SERVER_ERROR;
		send_response_message(client_fd, &response);
		return;
	}

	for (i = 0; i < NROWS; i++) {
		for (j = 0; j < NCOLS; j++) {
			temp[index] = game->board[i][j];
			index++;
		}
	}

	snprintf(response.payload, MAX_RSP_SIZE, "%d%s%s%s", size, PAYLOAD_DELIM,
			temp, PAYLOAD_DELIM);

	response.error = MSG_RSP_ERROR_NONE;
	send_response_message(client_fd, &response);
}

/**
 * Handles a request to check whose turn it is now.
 * @param[in] client_fd File descriptor of a client that is currently served.
 * @param[in] game      Pointer to a game structure that is currently played.
 * \sa game_s
 */
void
thread_handle_check_turn_request(int client_fd, game_s *game) {
	response_s response;
	response.type = MSG_CHECK_TURN_RSP;

	if (game->current_player == client_fd) {
		strncpy(response.payload, "0", 2);
	} else {
		strncpy(response.payload, "1", 2);
	}

	response.error = MSG_RSP_ERROR_NONE;
	send_response_message(client_fd, &response);
}

/**
 * Handles a request to perform given move on the board. It also checks whether
 * the game has ended and then terminates the thread.
 * @param[in] client_fd File descriptor of a client that is currently served.
 * @param[in] request   Pointer to a structure containing move coordinates details.
 * @param[in] game      Pointer to a game structure that is currently played.
 * \sa request_s game_s
 */
void thread_handle_make_move_request(int client_fd, request_s *request,
		game_s *game) {
	int lost, turn, validate_game = 0;
	char *result = NULL;
	response_s response;
	move_s move;
	response.type = MSG_MAKE_MOVE_RSP;

	if (game->current_player != client_fd) {
		response.error = MSG_RSP_ERROR_WRONG_TURN;
		send_response_message(client_fd, &response);
		return;
	}

	result = strtok(request->payload, PAYLOAD_DELIM);
	move.x = atoi(result) - 1;
	result = strtok(NULL, PAYLOAD_DELIM);
	move.y = atoi(result) - 1;
	get_pawn(client_fd, game, &move.pawn);
	validate_game = make_move(game->board, &move, &game->free);
	if (validate_game == -1) {
		response.error = MSG_RSP_ERROR_WRONG_MOVE;
		send_response_message(client_fd, &response);
		return;
	} else if (validate_game == 1) {
		response_s response_lst;
		response_lst.type = MSG_PRINT_LOST_RSP;
		response_lst.error = MSG_RSP_ERROR_NONE;
		response.type = MSG_PRINT_WIN_RSP;
		response.error = MSG_RSP_ERROR_NONE;
		send_broadcast_win_message(client_fd);
		send_response_message(client_fd, &response);

		if ((lost = check_current_player(client_fd)) != -1) {
			send_response_message(lost, &response_lst);
		}
		play = 0;
		thwork = 0;
		return;
	} else if (validate_game == 2) {
		send_broadcast_draw_message();
		play = 0;
		thwork = 0;
		return;
	}
	if ((turn = check_current_player(client_fd)) != -1) {
		game->current_player = turn;
	}

	response.error = MSG_RSP_ERROR_NONE;
	send_response_message(client_fd, &response);
	send_broadcast_message();
}

/**
 * Handles a request to forward a private message from a player to the opponent.
 * @param[in] client_fd File descriptor of a client that is currently served.
 * @param[in] request   Pointer to a structure containing a message for the opponent.
 * @param[in] game      Pointer to a game structure that is currently played.
 * \sa request_s game_s
 */
void
thread_handle_leave_message_request(int client_fd, request_s *request,
		game_s *game) {
	response_s response;
	response.type = MSG_LEAVE_MESSAGE_RSP;
	strncpy(response.payload, request->payload, MAX_RSP_SIZE);
	response.error = MSG_RSP_ERROR_NONE;
	if (client_fd == game->players[0]->player_fd) {
		send_response_message(game->players[1]->player_fd, &response);
	}
	if (client_fd == game->players[1]->player_fd) {
		send_response_message(game->players[0]->player_fd, &response);
	}
}

/**
 * Handles a request to give up. When a player gives up the game is finished
 * and thread is ended.
 * @param[in] client_fd  File descriptor of a client that is currently served.
 * @param[in] tbase_rdfs Bit array holding file descriptors being served by the current thread.
 */
void
thread_handle_giveup_request(int client_fd, fd_set *tbase_rdfs) {
	response_s response;
	update_connected_players(client_fd);
	FD_CLR(client_fd, tbase_rdfs);
	FD_SET(client_fd, tdata.rd_fds);
	response.type = MSG_LEAVE_RSP;
	response.error = MSG_RSP_ERROR_NONE;
	send_response_message(client_fd, &response);
}

/**
 * Handles a request to return to main menu. It can be sent by a spectator connected to a game.
 * @param[in] client_fd  File descriptor of a client that is currently served.
 * @param[in] tbase_rdfs Bit array holding file descriptors being served by the current thread.
 */
void
thread_handle_back_to_menu_request(int client_fd, fd_set *tbase_rdfs) {
	int i;
	response_s response;
	response.type = MSG_BACK_TO_MENU_RSP;
	response.error = MSG_RSP_ERROR_NONE;
	for (i = 0; i < SPECTATORS_NO; i++) {
		if (tdata.spectators_fd[i] == client_fd) {
			tdata.spectators_fd[i] = -1;
			tdata.game->no_connected_spectators--;
			FD_CLR(client_fd, tbase_rdfs);
			FD_SET(client_fd, tdata.rd_fds);
			send_response_message(client_fd, &response);
		}
	}
	printf("(Thread %d) Spectator disconnected\n", (int) pthread_self());
	kill(tdata.parent_pid, SIGRTMIN + 11);
}

/**
 * Checks request type and passes control to a function that serves a particular type of a message.
 * @param[in] client_fd  File descriptor of a client that is currently served.
 * @param[in] request    Pointer to a structure containing request data.
 * @param[in] game       Pointer to a game structure that is currently played.
 * @param[in] tbase_rdfs Bit array holding file descriptors being served by the current thread.
 * \sa game_s
 */
void
thread_request_handler(int client_fd, request_s *request, game_s *game,
		fd_set *tbase_rdfs) {
	switch (request->type) {
	case MSG_PRINT_BOARD_REQ:
		thread_handle_print_board_request(client_fd, game);
		break;
	case MSG_CHECK_TURN_REQ:
		thread_handle_check_turn_request(client_fd, game);
		break;
	case MSG_MAKE_MOVE_REQ:
		thread_handle_make_move_request(client_fd, request, game);
		break;
	case MSG_LEAVE_MESSAGE_REQ:
		thread_handle_leave_message_request(client_fd, request, game);
		break;
	case MSG_LEAVE_REQ:
		thread_handle_giveup_request(client_fd, tbase_rdfs);
		break;
	case MSG_BACK_TO_MENU_REQ:
		thread_handle_back_to_menu_request(client_fd, tbase_rdfs);
		break;
	default:
		break;
	}
}

/**
 * Handles incoming communication from clients and passes messages to be served.
 * @param[in] client_fd File descriptor of a client that is currently served.
 * @param[in] base_rdfs Bit array holding file descriptors being served by the current thread.
 * @param[in] tdata     Pointer to a structure containing arguments that are used by the thread.
 * \sa thread_data_s
 */
void
thread_communicate(int client_fd, fd_set *base_rdfs, thread_data_s *tdata) {
	char buffer[MAX_MSG_SIZE];
	ssize_t size;
	request_s request;
	pthread_t tid;
	memset(&request, 0, sizeof(request_s));
	tid = pthread_self();
	size = bulk_read(client_fd, buffer, MAX_MSG_SIZE);
	if (size == MAX_MSG_SIZE) {
		fprintf(stderr, "(Thread %d) Message received from client fd: %d\n",
				(int) tid, client_fd);
		string_to_request(buffer, &request);
		thread_request_handler(client_fd, &request, tdata->game, base_rdfs);
	}
	if (size == 0) {
		fprintf(stderr,
				"(Thread %d) End of file. Removing player. Closing descriptor: %d\n",
				(int) tid, client_fd);
		pthread_mutex_lock(tdata->players_list_mutex);
		remove_player_from_list2(tdata->players_list, client_fd);
		update_connected_players(client_fd);
		pthread_mutex_unlock(tdata->players_list_mutex);
		if (TEMP_FAILURE_RETRY(close(client_fd)) < 0) {
			ERR("close");
		}
		FD_CLR(client_fd, base_rdfs);

	}
	if (size < 0) {
		fprintf(stderr,
				"(Thread %d) Error. Removing player. Closing descriptor: %d\n",
				(int) tid, client_fd);
		pthread_mutex_lock(tdata->players_list_mutex);
		remove_player_from_list2(tdata->players_list, client_fd);
		update_connected_players(client_fd);
		pthread_mutex_unlock(tdata->players_list_mutex);
		if (TEMP_FAILURE_RETRY(close(client_fd)) < 0) {
			ERR("close");
		}
		FD_CLR(client_fd, base_rdfs);
	}
}

/**
 * Prepares file descriptors set by adding players and spectators file descriptors
 * and returns the maximum value that is held in the current set.
 * @param base_rdfs Bit array holding file descriptors being served by the current thread.
 * @return The maximum file descriptor value that is in the set.
 */
int
prepare_descriptor_set(fd_set *base_rdfs) {
	int i, fdmax = -1;
	FD_ZERO(base_rdfs);
	FD_SET(tdata.players_fd[0], base_rdfs);
	FD_SET(tdata.players_fd[1], base_rdfs);
	fdmax = max(tdata.players_fd[0], tdata.players_fd[1]);
	for (i = 0; i < SPECTATORS_NO; i++) {
		if (tdata.spectators_fd[i] != -1) {
			FD_SET(tdata.spectators_fd[i], base_rdfs);
			fdmax = max(fdmax, tdata.spectators_fd[i]);
		}
	}
	return fdmax;
}

/**
 * Main function of a thread.
 * @param thread_args Pointer to a structure containing arguments that are used by the thread.
 */
void
*thread_work(void *thread_args) {
	int i, fdmax;
	fd_set base_rdfs, rdfs;
	pthread_t tid;
	memcpy(&tdata, (thread_data_s*) thread_args, sizeof(thread_data_s));
	global_thread_rdfs = &base_rdfs;
	global_thrad_fdmax = &fdmax;
	tid = pthread_self();
	printf("Thread %d started\n", (int) tid);
	if ((fdmax = prepare_descriptor_set(&base_rdfs)) == -1) {
		fprintf(stderr, "(Thread %d) Unable to prepare descriptor set\n", (int) tid);
		exit(EXIT_FAILURE);
	}
	if (sethandler(sig_handler, SIGRTMIN + 1)) {
		ERR("Setting SIGRTMIN + 1:");
	}
	while (thwork) {
		rdfs = base_rdfs;
		if (pselect(fdmax + 1, &rdfs, NULL, NULL, NULL, NULL) > 0) {
			for (i = 0; i <= fdmax; i++) {
				if (FD_ISSET(i, &rdfs)) {
					thread_communicate(i, &base_rdfs, &tdata);
				}
			}
		} else {
			if (EINTR == errno)
				continue;
			ERR("Thread pselect:");
		}
	}
	pthread_cleanup_push(cleanup_handler, NULL);
	pthread_cleanup_pop(1);
	printf("Thread %d ended\n", (int) tid);
	pthread_exit(NULL);
	return NULL;
}

/**
 * Creates new structure for storing current thread information.
 * @param[out] new_thread A pointer to a structure holding information such
 * as game ID and thread ID.
 * @param[in]  thread Thread ID.
 * @param[in]  id     Game ID that a current thread will serve.
 * @retval  0 Upon successful thread structure creation.
 * @retval -1 When an error occurs.
 * \sa thread_s
 */
int
create_new_thread(thread_s **new_thread, pthread_t thread, int id) {
	(*new_thread) = malloc(sizeof(thread_s));
	if ((*new_thread) == NULL) {
		fprintf(stderr, "Failed to allocate memory for new thread\n");
		return -1;
	}
	(*new_thread)->game_id = id;
	(*new_thread)->pthread = thread;
	return 0;
}

/**
 * Initializes new thread, adds it to a list and starts serving connected clients.
 * @param targs              Pointer to a structure containing arguments that will be used by the thread.
 * @param threads_list       Pointer to a list holding threads.
 * @param threads_list_mutex Pointer to a mutex guarding threads list.
 * \sa thread_data_s threads_list_s
 */
void
initialize_thread(thread_data_s *targs, threads_list_s *threads_list,
		pthread_mutex_t *threads_list_mutex) {
	pthread_t thread;
	pthread_attr_t attr;
	thread_s *threads = NULL;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&thread, &attr, thread_work, targs) != 0) {
		ERR("pthread_create");
	}
	create_new_thread(&threads, thread, targs->game->id);
	pthread_mutex_lock(threads_list_mutex);
	add_thread_to_list(threads_list, threads);
	pthread_mutex_unlock(threads_list_mutex);
	pthread_attr_destroy(&attr);
}
