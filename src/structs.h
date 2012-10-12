/**
 * @file structs.h
 * @ingroup structs
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 6, 2012
 *
 * @brief File containing structures definition.
 */

#ifndef STRUCTS_H_
#define STRUCTS_H_

#include "config.h"
#include "enums.h"

typedef struct request_s request_s;
typedef struct response_s response_s;
typedef struct player_s player_s;
typedef struct players_list_s players_list_s;
typedef struct game_s game_s;
typedef struct games_list_s games_list_s;
typedef struct move_s move_s;
typedef struct thread_s thread_s;
typedef struct threads_list_s threads_list_s;
typedef struct thread_data_s thread_data_s;

/*!
 * \brief A structure to represent request message.
 */
struct request_s {
	/*@{*/
	message_type_e type; /**< The message type value. */
	char payload[MAX_REQ_SIZE]; /**< The payload of the message. */
	/*@}*/
};

/*!
 * \brief A structure to represent response message.
 */
struct response_s {
	/*@{*/
	message_type_e type; /**< The message type value. */
	message_error_e error; /**< The message error type value. */
	char payload[MAX_RSP_SIZE]; /**< The payload of the message. */
	/*@}*/
};

/*!
 * \brief A structure to represent a player.
 */
struct player_s {
	/*@{*/
	int player_fd; /**< Player file descriptor. */
	int game_id; /**< Game ID which players wants to play. */
	char player_nick[MAX_NICK_LEN]; /**< Player's nick name. */
	/*@}*/
};

/*!
 * \brief A structure to represent a players list.
 */
struct players_list_s {
	/*@{*/
	player_s *value; /**< Player structure. \sa player_s */
	struct players_list_s *next; /**< The pointer to the next list value. */
	/*@}*/
};

/*!
 * \brief A structure to represent a game structure.
 */
struct game_s {
	/*@{*/
	int id; /**< Game ID. */
	int free; /**< Number of spectator places left. \sa SPECTATORS_NO */
	int current_player; /**< The current player. */
	int no_connected_players; /**< Number of connected players. */
	int no_connected_spectators; /**< Number of connected spectators. */
	char **board; /**< Pointer to a board. */
	game_state_e state; /**< Current game state. */
	player_s *players[2]; /**< Array of size 2 containing player structures. \sa player_s */
	int spectators[SPECTATORS_NO]; /**< Array containing file descriptors of connected spectators. */
	/*@}*/
};

/*!
 * \brief A structure to represent a games list.
 */
struct games_list_s {
	/*@{*/
	game_s *value; /**< Game structure. \sa game_s */
	struct games_list_s *next; /**< The pointer to the next list value. */
	/*@}*/
};

/*!
 * \brief A structure to represent move coordinates.
 */
struct move_s {
	/*@{*/
	int x; /**< The x coordinate. */
	int y; /**< The y coordinate. */
	char pawn; /**< The pawn of the player. */
	/*@}*/
};

/*!
 * \brief A structure to represent thread data.
 */
struct thread_s {
	/*@{*/
	int game_id; /**< The game ID which is being served by current thread. */
	pthread_t pthread; /**< The thread ID. */
	/*@}*/
};

/*!
 * \brief A structure to represent threads list.
 */
struct threads_list_s {
	/*@{*/
	thread_s *value; /**< Thread structure. \sa thread_s */
	struct threads_list_s *next; /**< The pointer to the next list value. */
	/*@}*/
};

/*!
 * \brief A structure to represent arguments passed to thread serving a game.
 */
struct thread_data_s {
	/*@{*/
	int players_fd[2]; /**< Array of size 2 containing players file descriptors. */
	int spectators_fd[SPECTATORS_NO]; /**< Array containing file descriptors of connected spectators. */
	pid_t parent_pid; /**< The PID of the main server process. */
	pthread_mutex_t *players_list_mutex; /**< Pointer to the players list mutex. */
	pthread_mutex_t *games_list_mutex; /**< Pointer to the games list mutex. */
	pthread_mutex_t *threads_list_mutex; /**< Pointer to the threads list mutex. */
	fd_set *rd_fds; /**< Pointer to the base server file descriptor set. */
	game_s *game; /**< Pointer to the game structure. \sa game_s */
	games_list_s **games_list; /**< Pointer to the games list. \sa games_list_s */
	players_list_s **players_list; /**< Pointer to the players list. \sa players_list_s */
	threads_list_s **threads_list; /**< Pointer to the threads list. \sa threads_list_s */
	/*@}*/
};

#endif /* STRUCTS_H_ */
