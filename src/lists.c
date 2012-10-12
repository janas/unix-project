/**
 * @file lists.c
 * @ingroup lists
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 11, 2012
 *
 * @brief File containing methods for creating and manipulating lists of players, games and threads.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"

/***** Players list methods *****/

/**
 * Creates list for players.
 * @return Pointer to the head of the players list. \sa players_list_s
 */
players_list_s*
create_players_list(void) {
	players_list_s *players_list = NULL;
	players_list = malloc(sizeof(players_list_s));
	if (players_list == NULL) {
		fprintf(stderr, "Cannot allocate memory for players list\n");
		return NULL;
	}
	players_list->value = NULL;
	players_list->next = NULL;
	return players_list;
}

/**
 * Adds new player to a list.
 * @param[in] players_list Pointer to the head of the players list.
 * @param[in] player       Pointer to a structure containing player information.
 * @retval  0 Upon successful adding new player to the list.
 * @retval -1 When an error occurs.
 * \sa player_s
 */
int
add_player_to_list(players_list_s *players_list, player_s *player) {
	players_list_s *list, *new_list;
	list = players_list;

	if (list->value == NULL) {
		list->value = player;
		return 0;
	}

	while (list->next != NULL) {
		list = list->next;
	}

	new_list = create_players_list();
	if (new_list == NULL) {
		return -1;
	}

	list->next = new_list;
	new_list->value = player;
	return 0;
}

/**
 * Searches a players list to find a player in with a given nick.
 * @param[in] players_list Pointer to the head of the players list.
 * @param[in] nick         Nick name of a player to find.
 * @retval  0 When a player with a given nick is found in a list.
 * @retval -1 When a player with a given nick is not found in a list.
 */
int
find_player_by_nick(players_list_s *players_list, char *nick) {
	players_list_s *list = players_list;
	while (list != NULL && list->value != NULL) {
		if (strncmp(list->value->player_nick, nick, MAX_NICK_LEN) == 0) {
			return 0;
		} else {
			list = list->next;
		}
	}
	return -1;
}

/**
 * Searches a players list to find a player with a given file descriptor. When the player is
 * found a pointer to a structure is returned, NULL otherwise.
 * @param[in]  players_list Pointer to the head of the players list.
 * @param[out] player       Pointer to a structure which points to a found player.
 * @param[in]  client_fd    File descriptor number of a searched player.
 * \sa player_s
 */
void
get_player_by_file_desc(players_list_s *players_list, player_s **player, int client_fd) {
	players_list_s *list = players_list;
	while (list != NULL && list->value != NULL) {
		if (list->value->player_fd == client_fd) {
			*player = list->value;
			return;
		} else {
			list = list->next;
		}
	}
	*player = NULL;
}

/**
 * Removes given player structure from a players list.
 * @param[in] players_list Pointer to the head of the players list.
 * @param[in] player       Pointer to a structure which contains player to remove.
 * \sa player_s
 */
void
remove_player_from_list(players_list_s **players_list, player_s *player) {
	players_list_s *prev, *list = *players_list;
	if (list == NULL || list->value == NULL) {
		return;
	}
	if (list->next == NULL) {
		if (strncmp(list->value->player_nick, player->player_nick, MAX_NICK_LEN) == 0) {
			free(list->value);
			list->value = NULL;
		}
		return;
	}
	prev = NULL;
	while (list !=NULL && list->value != NULL) {
		if (strncmp(list->value->player_nick, player->player_nick, MAX_NICK_LEN) == 0) {
			/* delete first */
			if (prev == NULL) {
				free(list->value);
				*players_list = list->next;
				return;
			}
			/* delete last */
			if (list->next == NULL) {
				prev->next = NULL;
				free(list->value);
				return;
			}
			/* delete middle */
			prev->next = list->next;
			free(list->value);
			return;
		}
		prev = list;
		list = list->next;
	}
}

/**
 * Removes from a players list given player structure based on player's file descriptor.
 * @param[in] players_list Pointer to the head of the players list.
 * @param[in] client_fd    File descriptor number of a player to remove.
 */
void
remove_player_from_list2(players_list_s **players_list, int client_fd) {
	player_s *player;
	get_player_by_file_desc(*players_list, &player, client_fd);
	if (player != NULL) {
		remove_player_from_list(players_list, player);
	} else {
		fprintf(stderr, "Deletion error! Player not found in a list\n");
	}
}

/**
 * Destroys a players list by freeing all the pointers.
 * @param[in] players_list Pointer to the head of the players list.
 */
void
destroy_players(players_list_s *players_list) {
	players_list_s *next_list = players_list->next;
	while (next_list != NULL) {
		free(players_list);
		players_list = next_list;
		next_list = next_list->next;
	}

	free(players_list);
	players_list = NULL;
}

/***** Games list methods *****/

/**
 * Creates list for games.
 * @return Pointer to the head of the games list.
 * \sa games_list_s
 */
games_list_s*
create_games_list(void) {
	games_list_s *games_list = NULL;
	games_list = malloc(sizeof(games_list_s));
	if (games_list == NULL) {
		fprintf(stderr, "Cannot allocate memory for games list\n");
		return NULL;
	}
	games_list->value = NULL;
	games_list->next = NULL;
	return games_list;
}

/**
 * Adds new game to a list.
 * @param[in] games_list Pointer to the head of the games list.
 * @param[in] game       Pointer to a structure containing game information.
 * @retval  0 Upon successful adding new game to the list.
 * @retval -1 When an error occurs.
 * \sa game_s
 */
int
add_game_to_list(games_list_s *games_list, game_s *game) {
	games_list_s *list, *new_list;
	list = games_list;

	if (list->value == NULL) {
		list->value = game;
		return 0;
	}

	while (list->next != NULL) {
		list = list->next;
	}

	new_list = create_games_list();
	if (new_list == NULL) {
		return -1;
	}

	list->next = new_list;
	new_list->value = game;
	return 0;
}

/**
 * Searches a games list to find a game with a given game ID. When the game is
 * found a pointer to a structure is returned, NULL otherwise.
 * @param[in]  games_list Pointer to the head of the games list.
 * @param[out] game       Pointer to a structure which points to a found game.
 * @param[in]  game_id    Game ID which should be found.
 * \sa game_s
 */
void
get_game_by_id(games_list_s *games_list, game_s **game, int game_id) {
	games_list_s *list = games_list;
	while (list != NULL && list->value != NULL) {
		if (list->value->id == game_id) {
			*game = list->value;
			return;
		} else {
			list = list->next;
		}
	}
	*game = NULL;
}

/**
 * Removes given game structure from a games list.
 * @param[in] games_list Pointer to the head of the games list.
 * @param[in] game       Pointer to a structure which contains game to remove.
 * \sa game_s
 */
void
remove_game_from_list(games_list_s **games_list, game_s *game) {
	games_list_s *prev, *list = *games_list;
	if (list == NULL || list->value == NULL) {
		return;
	}
	if (list->next == NULL) {
		if (list->value->id == game->id) {
			free(list->value);
			list->value = NULL;
		}
		return;
	}
	prev = NULL;
	while (list != NULL && list->value != NULL) {
		if (list->value->id == game->id) {
			/* delete first */
			if (prev == NULL) {
				free(list->value);
				*games_list = list->next;
				return;
			}
			/* delete last */
			if (list->next == NULL) {
				prev->next = NULL;
				free(list->value);
				return;
			}
			/* delete middle */
			prev->next = list->next;
			free(list->value);
			return;
		}
		prev = list;
		list = list->next;
	}
}

/**
 * Destroys a games list by freeing all the pointers.
 * @param[in] games_list Pointer to the head of the games list.
 */
void
destroy_games(games_list_s *games_list) {
	games_list_s *next_list = games_list->next;
	while (next_list != NULL) {
		free(games_list);
		games_list = next_list;
		next_list = next_list->next;
	}

	free(games_list);
	games_list = NULL;
}

/***** Thread list methods *****/

/**
 * Creates list for threads.
 * @return Pointer to the head of the threads list.
 * \sa threads_list_s
 */
threads_list_s*
create_threads_list(void) {
	threads_list_s *threads_list = NULL;
	threads_list = malloc(sizeof(thread_s));
	if (threads_list == NULL) {
		fprintf(stderr, "Cannot allocate memory for threads list\n");
		return NULL;
	}
	threads_list->value = NULL;
	threads_list->next = NULL;
	return threads_list;
}

/**
 * Adds new thread to a list.
 * @param[in] threads_list Pointer to the head of the threads list.
 * @param[in] thread       Pointer to a structure containing thread information.
 * @retval  0 Upon successful adding new thread to the list.
 * @retval -1 When an error occurs.
 * \sa thread_s
 */
int
add_thread_to_list(threads_list_s *threads_list, thread_s *thread) {
	threads_list_s *list, *new_list;
	list = threads_list;

	if (list->value == NULL) {
		list->value = thread;
		return 0;
	}

	while (list->next != NULL) {
		list = list->next;
	}

	new_list = create_threads_list();
	if (new_list == NULL) {
		return -1;
	}

	list->next = new_list;
	new_list->value = thread;
	return 0;
}

/**
 * Searches a threads list to find a thread with a given game ID. When the thread is
 * found a pointer to a structure is returned, NULL otherwise.
 * @param[in]  threads_list Pointer to the head of the threads list.
 * @param[out] thread       Pointer to a structure which points to a found thread.
 * @param[in]  id           Game ID which should be found.
 * \sa thread_s
 */
void
get_thread_by_id(threads_list_s *threads_list, thread_s **thread, int id) {
	threads_list_s *list = threads_list;
	while (list != NULL && list->value != NULL) {
		if (list->value->game_id == id) {
			*thread = list->value;
			return;
		} else {
			list = list->next;
		}
	}
	*thread = NULL;
}

/**
 * Removes given thread structure from a threads list.
 * @param[in] threads_list Pointer to the head of the threads list.
 * @param[in] thread       Pointer to a structure which contains thread to remove.
 * \sa thread_s
 */
void
remove_thread_from_list(threads_list_s **threads_list, thread_s *thread) {
	threads_list_s *prev, *list = *threads_list;
	if (list == NULL || list->value == NULL) {
		return;
	}
	if (list->next == NULL) {
		if (list->value->game_id == thread->game_id) {
			free(list->value);
			list->value = NULL;
		}
		return;
	}
	prev = NULL;
	while (list !=NULL && list->value != NULL) {
		if (list->value->game_id == thread->game_id) {
			/* delete first */
			if (prev == NULL) {
				free(list->value);
				*threads_list = list->next;
				return;
			}
			/* delete last */
			if (list->next == NULL) {
				prev->next = NULL;
				free(list->value);
				return;
			}
			/* delete middle */
			prev->next = list->next;
			free(list->value);
			return;
		}
		prev = list;
		list = list->next;
	}
}

/**
 * Destroys a threads list by freeing all the pointers.
 * @param[in] threads_list Pointer to the head of the threads list.
 */
void
destroy_threads(threads_list_s *threads_list) {
	threads_list_s *next_list = threads_list->next;
	while (next_list != NULL) {
		free(threads_list);
		threads_list = next_list;
		next_list = next_list->next;
	}

	free(threads_list);
	threads_list = NULL;
}
