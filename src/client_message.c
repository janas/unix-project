/**
 * @file client_message.c
 * @ingroup client_message
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Aug 30, 2012
 *
 * @brief File containing methods responsible for displaying incoming messages (without sending a request).
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board_handler.h"
#include "common.h"
#include "messenger.h"
#include "request_sender.h"

/**
 * Prints out current state of a board.
 * @param[in] response Pointer to a message containing board information.
 */
void
get_print_board_message(response_s *response) {
	if (response->error != MSG_RSP_ERROR_NONE) {
		print_error_message(response->error);
		return;
	}
	print_spectator_board(response);
}

/**
 * Prints out a message from an opponent (private chat).
 * @param[in] response Pointer to a message containing text from the opponent.
 */
void
get_message_from_opponent(response_s *response) {
	if (response->error != MSG_RSP_ERROR_NONE) {
		print_error_message(response->error);
		return;
	}
	printf("\n\nMessage from the opponent: ");
	printf("%s\n", response->payload);
}

/**
 * Prints out a message after a game has ended or one of players has given up.
 * Then changes player mode into logged in menu.
 * @param[in] response Pointer to a message containing error.
 * @param[in] mode     Pointer to the current mode of a menu level.
 * \sa player_mode_e
 */
void
get_cleanup_message(response_s *response, player_mode_e *mode) {
	if (response->error != MSG_RSP_ERROR_NONE) {
		print_error_message(response->error);
		return;
	}
	if (*mode == PLAYER_MODE_CONNECTED) {
		printf("\n\nYour opponent has given up. Back to main menu\n");
	} else if (*mode == PLAYER_MODE_SPECTATOR) {
		printf("\n\nThe game has ended. Back to main menu\n");
	}
	*mode = PLAYER_MODE_LOGGED_IN;
}

/**
 * Prints out a message showing the winner player.
 * @param[in] response Pointer to a message containing error.
 * @param[in] mode     Pointer to the current mode of a menu level.
 */
void
get_print_result_message(response_s *response, player_mode_e *mode) {
	if (response->error != MSG_RSP_ERROR_NONE) {
		print_error_message(response->error);
		return;
	}
	printf("\n\n%s\n", response->payload);
	*mode = PLAYER_MODE_LOGGED_IN;
}

/**
 * Prints out a message showing the player that lost the game.
 * @param[in] response Pointer to a message containing error.
 * @param[in] mode     Pointer to the current mode of a menu level.
 */
void
get_print_lost_message(response_s *response, player_mode_e *mode) {
	if (response->error != MSG_RSP_ERROR_NONE) {
		print_error_message(response->error);
		return;
	}
	printf("\n\nYou lost the game!\n");
	*mode = PLAYER_MODE_LOGGED_IN;
}

/**
 * Prints out a message showing that the game has ended with a draw.
 * @param[in] response Pointer to a message containing error.
 * @param[in] mode     Pointer to the current mode of a menu level.
 */
void
get_print_draw_message(response_s *response, player_mode_e *mode) {
	if (response->error != MSG_RSP_ERROR_NONE) {
		print_error_message(response->error);
		return;
	}
	printf("\n\nThere is a draw! Game has ended.\n");
	*mode = PLAYER_MODE_LOGGED_IN;
}
