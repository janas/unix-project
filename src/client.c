/**
 * @file client.c
 * @ingroup client
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 6, 2012
 *
 * @brief File containing main client application code.
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
#include <fcntl.h>

#include "client_message.h"
#include "config.h"
#include "common.h"
#include "enums.h"
#include "messenger.h"
#include "request_sender.h"
#include "structs.h"

/*! \def HERR(source)
 * Macro for printing an error and its source.
 */
#define HERR(source) (fprintf(stderr,"%s(%d) at %s:%d\n",source,h_errno,__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

/**
 * A variable responsible for setting the value determining the main program loop termination.
 */
volatile sig_atomic_t work = 1;

/**
 * Prints out parameters required to start the application
 * @param[in] name Name of the client application file.
 */
void
usage(char* name) {
	fprintf(stderr, "Usage: %s host port\n", name);
	fprintf(stderr, "host - address to server\n");
	fprintf(stderr, "port - port to connect to\n");
}

/**
 * Initializes and fills structure containing an IP socket address
 * @param[in] address Address of the server.
 * @param[in] port    Port on which to connect to the server.
 * @return A structure containing the IP socket address.
 */
struct sockaddr_in
make_address(char *address, uint16_t port) {
	struct sockaddr_in addr;
	struct hostent *hostinfo;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	hostinfo = gethostbyname(address);
	if (hostinfo == NULL)
		HERR("gethostbyname");
	addr.sin_addr = *(struct in_addr*) hostinfo->h_addr;
	return addr;
}

/**
 * Connects to a server socket.
 * @param[in] name Address of the server.
 * @param[in] port Port on which to connect to the server.
 * @return Socket file descriptor number.
 */
int
connect_socket(char *name, uint16_t port) {
	struct sockaddr_in addr;
	int socketfd;
	socketfd = make_socket(PF_INET, SOCK_STREAM);
	addr = make_address(name, port);
	if (connect(socketfd, (struct sockaddr*) &addr, sizeof(struct sockaddr_in))
			< 0) {
		if (errno != EINTR) {
			ERR("connect");
		} else {
			fd_set wfds;
			int status;
			socklen_t size = sizeof(int);
			FD_ZERO(&wfds);
			FD_SET(socketfd, &wfds);
			if (TEMP_FAILURE_RETRY(
					select(socketfd + 1, NULL, &wfds, NULL, NULL)) < 0) {
				ERR("select");
			}
			if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &status, &size)
					< 0) {
				ERR("getsockopt");
			}
			if (0 != status) {
				ERR("connect");
			}
		}
	}
	return socketfd;
}

/**
 * Prints out menu options depending on a current menu level.
 * @param[in] mode Menu level that will be displayed.
 */
void
print_menu(player_mode_e mode) {
	printf("\n*** FOUR IN A LINE ***\n");
	switch (mode) {
	case PLAYER_MODE_START:
		printf("1 - Log in to the server\n");
		break;
	case PLAYER_MODE_LOGGED_IN:
		printf("1 - Print list of players\n");
		printf("2 - Print list of games\n");
		printf("3 - Create new game\n");
		printf("4 - Connect to an existing game\n");
		printf("5 - Connect to a game as spectator\n");
		break;
	case PLAYER_MODE_CONNECTED:
		printf("1 - Print board\n");
		printf("2 - Check whose turn is it\n");
		printf("3 - Make move\n");
		printf("4 - Leave a message\n");
		printf("5 - Give up\n");
		break;
	case PLAYER_MODE_SPECTATOR:
		printf("1 - Back to main menu\n");
		break;
	}
	printf("9 - Exit game\n");
	printf("\nChoose an option: ");
}

/**
 * Prints out error message when a wrong option is selected.
 */
void
print_choice_error(void) {
	printf("\nWrong option. Please enter correct option and try again.\n");
}

/**
 * Handles menu choices chosen by the user.
 * @param[in] choice        Option number chosen by the user.
 * @param[in] server_socket File descriptor of the socket connected to the server.
 * @param[in] game_id       Pointer to a game ID that a user is currently connected to.
 * @param[in] current_mode  Pointer to the current mode of a menu level.
 */
void
choice_handler(int choice, int server_socket, int *game_id, player_mode_e *current_mode) {
	if (choice == 9) {
		work = 0;
	} else if (*current_mode == PLAYER_MODE_START) {
		switch (choice) {
		case 1:
			send_game_login_request(server_socket, current_mode);
			break;
		default:
			print_choice_error();
			break;
		}
	} else if (*current_mode == PLAYER_MODE_LOGGED_IN) {
		switch (choice) {
		case 1:
			send_players_list_request(server_socket);
			break;
		case 2:
			send_games_list_request(server_socket);
			break;
		case 3:
			send_create_game_request(server_socket, current_mode, game_id);
			break;
		case 4:
			send_connect_game_request(server_socket, current_mode, game_id);
			break;
		case 5:
			send_connect_spectator_request(server_socket, current_mode, game_id);
			break;
		default:
			print_choice_error();
			break;
		}
	} else if (*current_mode == PLAYER_MODE_CONNECTED) {
		switch (choice) {
		case 1:
			send_print_board_request(server_socket);
			break;
		case 2:
			send_check_turn_request(server_socket);
			break;
		case 3:
			send_make_move_request(server_socket, current_mode);
			break;
		case 4:
			send_leave_message_request(server_socket);
			break;
		case 5:
			send_giveup_request(server_socket, current_mode, game_id);
			break;
		default:
			print_choice_error();
			break;
		}
	} else if (*current_mode == PLAYER_MODE_SPECTATOR) {
		switch (choice) {
		case 1:
			send_back_to_menu_request(server_socket, current_mode, game_id);
			break;
		default:
			print_choice_error();
			break;
		}
	}
}

/**
 * Handles incoming messages from a server without sending a request.
 * @param[in] server_socket File descriptor of the socket connected to the server.
 * @param[in] current_mode  Pointer to the current mode of a menu level.
 */
void
handle_incoming_message(int server_socket, player_mode_e *current_mode) {
	response_s response;
	receive_response_message(server_socket, &response);
	switch (response.type) {
	case MSG_PRINT_BOARD_SPC_RSP:
		get_print_board_message(&response);
		break;
	case MSG_LEAVE_MESSAGE_RSP:
		get_message_from_opponent(&response);
		break;
	case MSG_CLEANUP_RSP:
		get_cleanup_message(&response, current_mode);
		break;
	case MSG_PRINT_RESULT_SPC_RSP:
		get_print_result_message(&response, current_mode);
		break;
	case MSG_PRINT_LOST_RSP:
		get_print_lost_message(&response, current_mode);
		break;
	case MSG_PRINT_DRAW_RSP:
		get_print_draw_message(&response, current_mode);
		break;
	default:
		break;
	}
}

/**
 * Main function of the client application. It does a infinite loop unless a user exits the program.
 * @param[in] server_socket File descriptor of the socket connected to the server.
 */
void
doClient(int server_socket) {
	int fdmax, game_id = -1;
	char choice[HEADER];
	player_mode_e current_mode;
	fd_set rdfs, base;
	FD_ZERO(&base);
	FD_SET(server_socket, &base);
	FD_SET(STDIN_FILENO, &base);
	fdmax = server_socket;
	current_mode = PLAYER_MODE_START;
	do {
		rdfs = base;
		print_menu(current_mode);
		fflush(stdout);
		if (pselect(fdmax + 1, &rdfs, NULL, NULL, NULL, NULL) > 0) {
			if (FD_ISSET(server_socket, &rdfs)) {
				handle_incoming_message(server_socket, &current_mode);
			}
			if (FD_ISSET(STDIN_FILENO, &rdfs)) {
				read_line(choice, HEADER);
				choice_handler(atoi(choice), server_socket, &game_id,
						&current_mode);
			}
		}
	} while (work);
}

/**
 * The main procedure.
 * @param argc The command line.
 * @param argv The number of options in the command line.
 * @retval EXIT_SUCCESS Upon successful termination.
 * @retval EXIT_FAILURE When an error occurs.
 */
int
main(int argc, char **argv) {
	int port, server_socket;

	if (argc != 3) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	port = atoi(argv[2]);
	if (port <= 0 || port > 65535) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	server_socket = connect_socket(argv[1], port);
	doClient(server_socket);

	if (TEMP_FAILURE_RETRY(close(server_socket)) < 0) {
		ERR("close");
	}
	return EXIT_SUCCESS;
}
