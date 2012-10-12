/**
 * @file server.c
 * @ingroup server
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 6, 2012
 *
 * @brief File containing main server application code.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include <fcntl.h>

#include "config.h"
#include "common.h"
#include "lists.h"
#include "messenger.h"
#include "request_handler.h"
#include "structs.h"

/**
 * A variable responsible for setting the value determining the main program loop termination.
 */
volatile sig_atomic_t work = 1;

/**
 * Prints out parameters required to start the application
 * @param[in] name Name of the server application file.
 */
void
usage(char *name) {
	fprintf(stderr, "Usage: %s port\n", name);
	fprintf(stderr, "port - port to listen\n");
}

/**
 * Serves SIGINT by setting main loop variable value to 0 which in turn terminates the program.
 * @param[in] sig A signal number that will be served.
 */
void
sigint_handler(int sig) {
	work = 0;
}

/**
 * Serves SIGRTMIN+11 by notifying the doServer function that a client has finished/resigned
 * from playing/observing a game.
 *@param[in] sig A signal number that will be served.
 */
void
sig_update_handler(int sig) {
	sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGRTMIN + 11);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	bulk_write(3, "0", 1);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

/**
 * Binds a socket and starts listening on incoming connections.
 * @param[in] port A port to listen on.
 * @param[in] type The type of the communication semantics.
 * @return File descriptor that a server will listen on.
 */
int
bind_inet_socket(uint16_t port, int type) {
	struct sockaddr_in addr;
	int socketfd, t = 1;
	socketfd = make_socket(PF_INET, type);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
		ERR("setsockopt");
	if (bind(socketfd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
		ERR("bind");
	if (SOCK_STREAM == type)
		if (listen(socketfd, BACKLOG) < 0)
			ERR("listen");
	return socketfd;
}

/**
 * Accepts new incoming connection from a client.
 * @param[in] sfd Accepts incoming connection, thus returning a file descriptor of a
 * newly connected client.
 * @return A file descriptor number of a newly connected client or -1 on error.
 */
int
add_new_client(int sfd) {
	int nfd;
	if ((nfd = TEMP_FAILURE_RETRY(accept(sfd, NULL, NULL))) < 0) {
		if (EAGAIN == errno || EWOULDBLOCK == errno)
			return -1;
		ERR("accept");
	}
	return nfd;
}

/**
 * Serves client request by checking a request type and calling appropriate function.
 * @param[in] client_fd          File descriptor of a client that is currently served.
 * @param     request            Pointer to a structure containing request data.
 * @param     base_rdfs          Bit array holding file descriptor to be served by the server.
 * @param     players_list       Pointer to a list holding players.
 * @param     games_list         Pointer to a list holding games.
 * @param     threads_list       Pointer to a list holding threads.
 * @param     players_list_mutex Pointer to a mutex guarding players list.
 * @param     games_list_mutex   Pointer to a mutex guarding games list.
 * @param     threads_list_mutex Pointer to a mutex guarding threads list.
 * \sa request_s players_list_s games_list_s threads_list_s message_type_e
 */
void
request_handler(int client_fd, request_s *request, fd_set *base_rdfs,
		players_list_s **players_list, games_list_s **games_list,
		threads_list_s **threads_list, pthread_mutex_t *players_list_mutex,
		pthread_mutex_t *games_list_mutex, pthread_mutex_t *threads_list_mutex) {
	switch (request->type) {
	case MSG_LOGIN_REQ:
		handle_game_login_request(client_fd, request, *players_list);
		break;
	case MSG_PLAYERS_LIST_REQ:
		handle_players_list_request(client_fd, players_list);
		break;
	case MSG_GAMES_LIST_REQ:
		handle_game_list_request(client_fd, games_list);
		break;
	case MSG_CREATE_GAME_REQ:
		handle_create_new_game_request(client_fd, request, *players_list,
				*games_list, games_list_mutex);
		break;
	case MSG_CONNECT_GAME_REQ:
		handle_connect_to_existing_game_request(client_fd, request, base_rdfs,
				players_list, games_list, threads_list, players_list_mutex,
				games_list_mutex, threads_list_mutex);
		break;
	case MSG_CONNECT_SPECTATOR_REQ:
		handle_connect_as_spectator_request(client_fd, request, base_rdfs,
				*games_list, *threads_list);
		break;
	case MSG_BACK_TO_MENU_REQ:
		handle_back_to_menu_request(client_fd, request, *games_list);
		break;
	case MSG_PRINT_BOARD_REQ:
		handle_game_message(client_fd, request);
		break;
	case MSG_CHECK_TURN_REQ:
		handle_game_message(client_fd, request);
		break;
	case MSG_MAKE_MOVE_REQ:
		handle_game_message(client_fd, request);
		break;
	case MSG_LEAVE_MESSAGE_REQ:
		handle_game_message(client_fd, request);
		break;
	case MSG_LEAVE_REQ:
		handle_leave_game_request(client_fd, request, games_list,
				games_list_mutex);
		break;
	default:
		break;
	}
}

/**
 * Reads data from a client socket and passes forward to handle a message or removes a player
 * and closes socket.
 * @param[in] client_fd          File descriptor of a client that is currently served.
 * @param     base_rdfs          Bit array holding file descriptors to be served by the server.
 * @param     players_list       Pointer to a list holding players.
 * @param     games_list         Pointer to a list holding games.
 * @param     threads_list       Pointer to a list holding threads.
 * @param     players_list_mutex Pointer to a mutex guarding players list.
 * @param     games_list_mutex   Pointer to a mutex guarding games list.
 * @param     threads_list_mutex Pointer to a mutex guarding threads list.
 */
void
communicate(int client_fd, fd_set *base_rdfs,
		players_list_s **players_list, games_list_s **games_list,
		threads_list_s **threads_list, pthread_mutex_t *players_list_mutex,
		pthread_mutex_t *games_list_mutex, pthread_mutex_t *threads_list_mutex) {
	char buffer[MAX_MSG_SIZE];
	ssize_t size;
	request_s request;
	memset(&request, 0, sizeof(request_s));
	size = bulk_read(client_fd, buffer, MAX_MSG_SIZE);
	if (size == MAX_MSG_SIZE) {
		fprintf(stderr, "Message received from fd: %d\n", client_fd);
		string_to_request(buffer, &request);
		request_handler(client_fd, &request, base_rdfs, players_list,
				games_list, threads_list, players_list_mutex, games_list_mutex,
				threads_list_mutex);
	}
	if (size == 0) {
		fprintf(stderr,
				"End of file. Removing player. Closing descriptor: %d\n",
				client_fd);
		pthread_mutex_lock(players_list_mutex);
		remove_player_from_list2(players_list, client_fd);
		pthread_mutex_unlock(players_list_mutex);
		if (TEMP_FAILURE_RETRY(close(client_fd)) < 0)
			ERR("close");
		FD_CLR(client_fd, base_rdfs);

	}
	if (size < 0) {
		fprintf(stderr, "Error. Removing player. Closing descriptor: %d\n",
				client_fd);
		pthread_mutex_lock(players_list_mutex);
		remove_player_from_list2(players_list, client_fd);
		pthread_mutex_unlock(players_list_mutex);
		if (TEMP_FAILURE_RETRY(close(client_fd)) < 0)
			ERR("close");
		FD_CLR(client_fd, base_rdfs);
	}
}

/**
 * Displays log messages containing IP address, port and a file descriptor
 * when new client connects.
 * @param[in] client_fd File descriptor of a socket that a client is connected to.
 */
void
display_log(int client_fd) {
	socklen_t addrlen;
	struct sockaddr_in addr;
	char buf[22];
	if (getpeername(client_fd, (struct sockaddr*) &addr, &addrlen) == 0) {
		snprintf(buf, sizeof(buf), "%s:%u", inet_ntoa(addr.sin_addr),
				(unsigned) ntohs(addr.sin_port));
		fprintf(stderr, "New client connected %s, fd %d\n", buf, client_fd);
	}
}

/**
 * Initializes needed structures.
 * @param players_list Pointer to a structure holding a list of players.
 * @param games_list   Pointer to a structure holding a list of games.
 * @param threads_list Pointer to a structure holding a lsit of threads.
 */
void
initialize_structures(players_list_s **players_list,
		games_list_s **games_list, threads_list_s **threads_list) {
	(*players_list) = create_players_list();
	if (players_list == NULL) {
		fprintf(stderr, "Error! Players list is not initialized\n");
		exit(EXIT_FAILURE);
	}
	(*games_list) = create_games_list();
	if (games_list == NULL) {
		fprintf(stderr, "Error! Games list is not initialized\n");
		exit(EXIT_FAILURE);
	}
	(*threads_list) = create_threads_list();
	if (threads_list == NULL) {
		fprintf(stderr, "Error! Threads list is not initialized\n");
		exit(EXIT_FAILURE);
	}
}

/**
 * Main function of the server application. It does a infinite loop unless a user exits the program.
 * @param[in] listener_socket File descriptor of the socket listening incoming connections.
 * @param[in] fifo            File descriptor of a temporary FIFO file.
 */
void
doServer(int listener_socket, int fifo) {
	int i, fdmax, newfd;
	players_list_s *players_list = NULL;
	games_list_s *games_list = NULL;
	threads_list_s *threads_list = NULL;
	pthread_mutex_t players_list_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t games_list_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t threads_list_mutex = PTHREAD_MUTEX_INITIALIZER;
	fd_set base_rdfs, rdfs;
	sigset_t mask, oldmask;
	FD_ZERO(&base_rdfs);
	FD_SET(listener_socket, &base_rdfs);
	fdmax = max(listener_socket, fifo);
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	initialize_structures(&players_list, &games_list, &threads_list);
	printf("Four-in-a-line server started\n");
	while (work) {
		rdfs = base_rdfs;
		if (pselect(fdmax + 1, &rdfs, NULL, NULL, NULL, &oldmask) > 0) {
			for (i = 0; i <= fdmax; i++) {
				newfd = -1;
				if (FD_ISSET(i, &rdfs)) {
					if (i == listener_socket) {
						/* request from newly connected client */
						newfd = add_new_client(listener_socket);
						if (newfd > 0) {
							FD_SET(newfd, &base_rdfs);
							if (newfd > fdmax) {
								fdmax = newfd;
							}
							display_log(newfd);
							communicate(newfd, &base_rdfs, &players_list,
									&games_list, &threads_list,
									&players_list_mutex, &games_list_mutex,
									&threads_list_mutex);
						} else if (i == fifo) {
							/* trick to update base_rdfs set */
							char temp[1];
							bulk_read(fifo, temp, 1);
						}
					} else {
						/* request from already connected client */
						communicate(i, &base_rdfs, &players_list, &games_list,
								&threads_list, &players_list_mutex,
								&games_list_mutex, &threads_list_mutex);
					}
				}
			}
		} else {
			if (EINTR == errno)
				continue;
			ERR("pselect");
		}
	}
	pthread_mutex_destroy(&players_list_mutex);
	pthread_mutex_destroy(&games_list_mutex);
	pthread_mutex_destroy(&threads_list_mutex);
	destroy_players(players_list);
	destroy_games(games_list);
	destroy_threads(threads_list);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
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
	int port, fifo, listener_socket;
	if (argc != 2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	port = atoi(argv[1]);
	if (port <= 0 || port > 65535) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (mkfifo(FIFO_NAME, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
		if (errno != EEXIST) {
			perror("Create fifo:");
			exit(EXIT_FAILURE);
		}
	if ((fifo = TEMP_FAILURE_RETRY(open(FIFO_NAME, O_RDWR))) < 0) {
		perror("Open fifo:");
		exit(EXIT_FAILURE);
	}
	if (sethandler(sigint_handler, SIGINT)) {
		ERR("Setting SIGINT:");
	}
	if (sethandler(sig_update_handler, SIGRTMIN + 11)) {
		ERR("Setting SIGRTMIN+11:");
	}

	listener_socket = bind_inet_socket(port, SOCK_STREAM);
	doServer(listener_socket, fifo);

	if (TEMP_FAILURE_RETRY(close(listener_socket)) < 0) {
		ERR("Close:");
	}
	if (TEMP_FAILURE_RETRY(close(fifo)) < 0) {
		ERR("Close fifo:");
	}
	if (TEMP_FAILURE_RETRY(unlink(FIFO_NAME)) < 0) {
		ERR("Unlink fifo:");
	}
	printf("Server has terminated normally.\n");
	return EXIT_SUCCESS;
}
