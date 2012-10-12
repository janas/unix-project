/**
 * @file lists.h
 * @ingroup lists
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 11, 2012
 *
 * @brief File containing methods for creating and manipulating lists of players, games and threads.
 */

#ifndef LISTS_H_
#define LISTS_H_

#include "structs.h"

players_list_s* create_players_list(void);
int add_player_to_list(players_list_s *players_list, player_s *player);
int find_player_by_nick(players_list_s *players_list, char *nick);
void get_player_by_file_desc(players_list_s *players_list, player_s **player, int client_fd);
void remove_player_from_list(players_list_s **players_list, player_s *player);
void remove_player_from_list2(players_list_s **players_list, int client_fd);
void destroy_players(players_list_s *players_list);

games_list_s* create_games_list(void);
int add_game_to_list(games_list_s *games_list, game_s *game);
void get_game_by_id(games_list_s *games_list, game_s **game, int game_id);
void remove_game_from_list(games_list_s **games_list, game_s *game);
void destroy_games(games_list_s *games_list);

threads_list_s* create_threads_list(void);
int add_thread_to_list(threads_list_s *threads_list, thread_s *thread);
void get_thread_by_id(threads_list_s *threads_list, thread_s **thread, int id);
void remove_thread_from_list(threads_list_s **threads_list, thread_s *thread);
void destroy_threads(threads_list_s *threads_list);

#endif /* LISTS_H_ */
