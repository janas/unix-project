/**
 * @file request_sender.c
 * @ingroup request_sender
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 12, 2012
 *
 * @brief File containing methods for sending requests to the server.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "messenger.h"

/**
 * Prints out error message that is read from server's response message.
 * @param[in] error The enumeration of message errors.
 * \sa message_error_e
 */
void
print_error_message(message_error_e error) {
	switch (error) {
	case MSG_RSP_ERROR_NONE:
		break;
	case MSG_RSP_ERROR_NICK_EXISTS:
		printf("\nNick exists on the game server. Please pick another one.\n");
		break;
	case MSG_RSP_INTERNAL_SERVER_ERROR:
		printf("\nInternal server error. Please try again.\n");
		break;
	case MSG_RSP_ERROR_WRONG_BORAD_SIZE:
		printf("\nWrong board size. Type correct size and try again.\n");
		break;
	case MSG_RSP_ERROR_WRONG_GAME_ID:
		printf("\nWrong game id. Type correct game id and try again.\n");
		break;
	case MSG_RSP_ERROR_TOO_MANY_PLAYERS:
		printf("\nThe game is full. You can connect as a spectator or choose another one.\n");
		break;
	case MSG_RSP_ERROR_TOO_MANY_SPECTATORS:
		printf("\nAll spectator places are currently occupied. Please try again later.\n");
		break;
	case MSG_RSP_ERROR_WRONG_TURN:
		printf("\nIt is not your turn. Wait for your opponent.\n");
		break;
	case MSG_RSP_ERROR_WRONG_MOVE:
		printf("\nCannot execute specified move. Please check your move and try again.\n");
		break;
	case MSG_RSP_ERROR_WAIT_OPPONENT:
		printf("\nWait for an opponent to connect.\n");
		break;
	}
}

/**
 * Prints out error message when transmission error occurs.
 */
void
print_transmission_error_message()
{
	printf("\nError occurred during transmission from server\n");
	printf("Please try again\n");
}

/**
 * Prints out a list of games that are currently on the server with such
 * details as game ID, board size, players' nicks and spectators.
 * @param[in] idx Number of a piece of information to be displayed.
 * @param[in] str Pointer to a character string containing message to be
 * formatted and displayed.
 */
void
print_games_list(int idx, char *str) {
	switch (idx) {
	case 0:
		printf("\n----------------------\n");
		printf("Game ID: %s\n", str);
		break;
	case 1:
		printf("Board size: %s\n", str);
		break;
	case 2:
		printf("Free spectators: %d\n", atoi(str));
		printf("Players: \n");
		break;
	case 3:
		printf("         1. %s\n", str);
		break;
	case 4:
		printf("         2. %s\n", str);
		break;
	default:
		break;
	}
}

/**
 * Prints out a divider when displaying a board. It is responsible
 * for horizontal separation of cells.
 * @param[in] size Size of the board.
 */
void
print_divider(int size) {
	int i;
	printf("+");
	for (i = 0; i < size; i++)
		printf("---+");
	printf("\n");
}

/**
 * Main function responsible for displaying a board.
 * @param[in] size  Size of the board.
 * @param[in] board Two dimensional array containing current state of a board.
 */
void
print_board(int size, char board[NROWS][NCOLS]) {
	int i, j;
	print_divider(size);
	for (i = 0; i < size; i++) {
		printf("|");
		for (j = 0; j < size; j++) {
			if (board[i][j] == '0') {
				continue;
			}
			if (board[i][j] == '1') {
				printf("   |");
			} else {
				printf(" %c |", board[i][j]);
			}
		}
		printf("\n");
		print_divider(size);
	}
}

/**
 * Sends client's login request to a server.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 * @param[in] mode      Menu level that will be displayed.
 */
void
send_game_login_request(int server_fd, player_mode_e *mode) {
	char nick[MAX_NICK_LEN];
	request_s request;
	response_s response;
	request.type = MSG_LOGIN_REQ;
	printf("Enter nick (max %d characters): ", MAX_NICK_LEN);
	read_line(nick, MAX_NICK_LEN);
	strncpy(request.payload, nick, MAX_NICK_LEN);
	send_receive_message(server_fd, &request, &response);

	if (response.type != MSG_LOGIN_RSP) {
		print_transmission_error_message();
		return;
	}
	if (response.error != MSG_RSP_ERROR_NONE) {
		print_error_message(response.error);
		return;
	}
	*mode = PLAYER_MODE_LOGGED_IN;
}

/**
 * Sends a request to the server to obtain a list of connected players.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 */
void
send_players_list_request(int server_fd) {
	int i = 0;
	char delims[] = PAYLOAD_DELIM;
	char *result = NULL;
	response_s response;
	request_s request;
	request.type = MSG_PLAYERS_LIST_REQ;
	send_receive_message(server_fd, &request, &response);
	if (response.type != MSG_PLAYERS_LIST_RSP) {
		print_transmission_error_message();
		return;
	}
	if (response.error != MSG_RSP_ERROR_NONE) {
		print_error_message(response.error);
		return;
	}

	result = strtok(response.payload, delims);
	printf("\nList of connected players:\n\n");
	while (result != NULL) {
		printf("%d: %s\n", i + 1, result);
		result = strtok(NULL, delims);
		i++;
	}

	printf("\nTotal %d players connected\n", i);
}

/**
 * Sends a request to the server to obtain a list of games currently
 * held on the server.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 */
void
send_games_list_request(int server_fd) {
	int i, j;
	char *saveptr1, *saveptr2, *token, *subtoken;
	char *str1, *str2;
	response_s response;
	request_s request;
	request.type = MSG_GAMES_LIST_REQ;
	send_receive_message(server_fd, &request, &response);
	if (response.type != MSG_GAMES_LIST_RSP) {
		print_transmission_error_message();
		return;
	}
	if (response.error != MSG_RSP_ERROR_NONE) {
		print_error_message(response.error);
		return;
	}

	printf("\nList of running games:\n");
	/*
	 * if this is true then there is no game at the
	 * server since there will not be id like 000000
	 */
	if (strncmp(response.payload, "000000", 6) == 0) {
		printf("\nCurrently there is no game at the server\n");
		return;
	}

	for (i = 0, str1 = response.payload;; i++, str1 = NULL) {
		token = strtok_r(str1, PAYLOAD_DELIM, &saveptr1);
		if (token == NULL) {
			break;
		}
		for (j = 0, str2 = token;; j++, str2 = NULL) {
			subtoken = strtok_r(str2, INNER_DELIM, &saveptr2);
			if (subtoken == NULL) {
				break;
			}
			print_games_list(j, subtoken);
		}
	}

	printf("\n----------------------\n");
	printf("Total %d running games\n", i);
}

/**
 * Sends a request to the server to create new game.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 * @param[in] mode      Menu level that will be displayed.
 * @param[in] game_id   Pointer to the game ID that new game will have.
 * \sa player_mode_e
 */
void
send_create_game_request(int server_fd, player_mode_e *mode, int *game_id) {
	char size[16];
	request_s request;
	response_s response;
	request.type = MSG_CREATE_GAME_REQ;
	printf("\nEnter board size (min %d, max %d): ", MIN_BOARD_SIZE,
			MAX_BOARD_SIZE);
	read_line(size, sizeof(size));
	strncpy(request.payload, size, sizeof(size));
	send_receive_message(server_fd, &request, &response);
	if (response.type != MSG_CREATE_GAME_RSP) {
		print_transmission_error_message();
		return;
	}
	if (response.error != MSG_RSP_ERROR_NONE) {
		print_error_message(response.error);
		return;
	}
	if (*mode == PLAYER_MODE_LOGGED_IN) {
		*mode = PLAYER_MODE_CONNECTED;
		*game_id = atoi(response.payload);
	}
}

/**
 * Sends a request to the server to connect to an existing game.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 * @param[in] mode      Menu level that will be displayed.
 * @param[in] game_id   Pointer that stores the game ID that a client will be connected to.
 * \sa player_mode_e
 */
void
send_connect_game_request(int server_fd, player_mode_e *mode, int *game_id) {
	char game_no[3];
	request_s request;
	response_s response;
	request.type = MSG_CONNECT_GAME_REQ;
	printf("Enter game id: ");
	read_line(game_no, 3);
	strncpy(request.payload, game_no, 3);
	send_receive_message(server_fd, &request, &response);
	if (response.type != MSG_CONNECT_GAME_RSP) {
		print_transmission_error_message();
		return;
	}
	if (response.error != MSG_RSP_ERROR_NONE) {
		print_error_message(response.error);
		return;
	}
	if (*mode == PLAYER_MODE_LOGGED_IN) {
		*mode = PLAYER_MODE_CONNECTED;
		*game_id = atoi(game_no);
	}
}

/**
 * Sends a request to the server to connect to an existing game as a spectator.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 * @param[in] mode      Menu level that will be displayed.
 * @param[in] game_id   Pointer that stores the game ID that a client (spectator)
 * will be connected to.
 * \sa player_mode_e
 */
void
send_connect_spectator_request(int server_fd, player_mode_e *mode,
		int *game_id) {
	char size[3];
	request_s request;
	response_s response;
	request.type = MSG_CONNECT_SPECTATOR_REQ;
	printf("Enter game id: ");
	read_line(size, 3);
	strncpy(request.payload, size, 3);
	send_receive_message(server_fd, &request, &response);
	if (response.type != MSG_CONNECT_SPECTATOR_RSP) {
		print_transmission_error_message();
		return;
	}
	if (response.error != MSG_RSP_ERROR_NONE) {
		print_error_message(response.error);
		return;
	}
	if (*mode == PLAYER_MODE_LOGGED_IN) {
		*mode = PLAYER_MODE_SPECTATOR;
		*game_id = atoi(size);
	}
}

/**
 * Sends a request to the server to print out current state of a board.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 */
void
send_print_board_request(int server_fd) {
	int i, j, ptr = 0;
	int size = 0;
	char temp[3], board[NROWS][NCOLS];
	char *result = NULL;
	request_s request;
	response_s response;
	request.type = MSG_PRINT_BOARD_REQ;
	send_receive_message(server_fd, &request, &response);
	if (response.type != MSG_PRINT_BOARD_RSP) {
		print_transmission_error_message();
		return;
	}
	if (response.error != MSG_RSP_ERROR_NONE) {
		print_error_message(response.error);
		return;
	}

	result = strtok(response.payload, PAYLOAD_DELIM);
	strncpy(temp, result, 3);
	size = atoi(temp);
	result = strtok(NULL, PAYLOAD_DELIM);
	for (i = 0; i < NROWS; i++)
		for (j = 0; j < NCOLS; j++) {
			board[i][j] = result[ptr];
			ptr++;
		}
	printf("\nCurrent board state:\n");
	print_board(size, board);
}

/**
 * Sends a request to the server to check whose turn it is now.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 */
void
send_check_turn_request(int server_fd) {
	int turn;
	char temp[2];
	char *result = NULL;
	request_s request;
	response_s response;
	request.type = MSG_CHECK_TURN_REQ;
	send_receive_message(server_fd, &request, &response);
	if (response.type != MSG_CHECK_TURN_RSP) {
		print_transmission_error_message();
		return;
	}
	if (response.error != MSG_RSP_ERROR_NONE) {
		print_error_message(response.error);
		return;
	}

	result = strtok(response.payload, PAYLOAD_DELIM);
	strncpy(temp, result, 2);
	turn = atoi(temp);
	if (turn == 0) {
		printf("\nIt's your turn\n");
		return;
	}
	if (turn == 1) {
		printf("\nIt's your opponent's turn\n");
		return;
	} else {
		printf("\nError during transmission\n");
		return;
	}
}

/**
 * Sends a request to the server to perform a move on the board. If
 * the game is won/lost or drawn it backs to main menu.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 * @param[in] mode      Menu level that will be displayed.
 * \sa player_mode_e
 */
void
send_make_move_request(int server_fd, player_mode_e *mode) {
	char x[3], y[3];
	request_s request;
	response_s response;
	request.type = MSG_MAKE_MOVE_REQ;
	printf("\nEnter x coordinate: ");
	read_line(x, 3);
	printf("Enter y coordinate: ");
	read_line(y, 3);
	snprintf(request.payload, 25, "%d%s%d%s", atoi(x), PAYLOAD_DELIM, atoi(y),
			PAYLOAD_DELIM);
	send_receive_message(server_fd, &request, &response);
	if (response.type != MSG_MAKE_MOVE_RSP) {
		if (response.type == MSG_PRINT_WIN_RSP) {
			printf("\nYou won the game!\n");
			*mode = PLAYER_MODE_LOGGED_IN;
		} else if (response.type == MSG_PRINT_DRAW_RSP) {
			printf("\nThere is a draw! Game has ended.\n");
			*mode = PLAYER_MODE_LOGGED_IN;
		} else {
			print_transmission_error_message();
			return;
		}
	}
	if (response.error != MSG_RSP_ERROR_NONE) {
		print_error_message(response.error);
		return;
	}
}

/**
 * Sends a request to the server containing private message for the opponent
 * to be forward to the recipient.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 */
void
send_leave_message_request(int server_fd) {
	char msg[MAX_RSP_SIZE - sizeof(MSG_DELIM)];
	request_s request;
	request.type = MSG_LEAVE_MESSAGE_REQ;
	printf("Enter message for opponent (max %lu chars): ", sizeof(msg));
	read_line(msg, sizeof(msg));
	strncpy(request.payload, msg, MAX_REQ_SIZE);
	send_request_message(server_fd, &request);
}

/**
 * Sends a request to the server to give up the game.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 * @param[in] mode      Menu level that will be displayed.
 * @param[in] game_id   Pointer that stores the game ID that a client is connected to.
 * \sa player_mode_e
 */
void
send_giveup_request(int server_fd, player_mode_e *mode, int *game_id) {
	request_s request;
	response_s response;
	request.type = MSG_LEAVE_REQ;
	snprintf(request.payload, 4, "%d", *game_id);
	send_receive_message(server_fd, &request, &response);
	if (response.type != MSG_LEAVE_RSP) {
		print_transmission_error_message();
		return;
	}
	if (response.error != MSG_RSP_ERROR_NONE) {
		print_error_message(response.error);
		return;
	}
	if (*mode == PLAYER_MODE_CONNECTED) {
		*mode = PLAYER_MODE_LOGGED_IN;
		*game_id = -1;
	}
}

/**
 * Sends a request to the server to back to main menu when connected as a spectator.
 * @param[in] server_fd File descriptor of the socket connected to the server.
 * @param[in] mode      Menu level that will be displayed.
 * @param[in] game_id   Pointer that stores the game ID that a client (spectator)
 * is connected to.
 * \sa player_mode_e
 */
void
send_back_to_menu_request(int server_fd, player_mode_e *mode, int *game_id) {
	request_s request;
	response_s response;
	request.type = MSG_BACK_TO_MENU_REQ;
	snprintf(request.payload, 5, "%d%s", *game_id, PAYLOAD_DELIM);
	send_receive_message(server_fd, &request, &response);
	if (response.type != MSG_BACK_TO_MENU_RSP) {
		print_transmission_error_message();
		return;
	}
	if (response.error != MSG_RSP_ERROR_NONE) {
		print_error_message(response.error);
		return;
	}
	if (*mode == PLAYER_MODE_SPECTATOR) {
		*mode = PLAYER_MODE_LOGGED_IN;
		*game_id = -1;
	}
}

/**
 * Prints out current state of the board that is sent to spectators.
 * @param[in] response Pointer to a structure containing response
 * message (current state of a board).
 * \sa response_s
 */
void
print_spectator_board(response_s *response) {
	int i, j, ptr = 0;
	int size = 0;
	char temp[3];
	char *result = NULL;
	char board[NROWS][NCOLS];
	result = strtok(response->payload, PAYLOAD_DELIM);
	strncpy(temp, result, 3);
	size = atoi(temp);
	result = strtok(NULL, PAYLOAD_DELIM);
	for (i = 0; i < NROWS; i++)
		for (j = 0; j < NCOLS; j++) {
			board[i][j] = result[ptr];
			ptr++;
		}
	printf("\n\nCurrent board state:\n");
	print_board(size, board);
}
