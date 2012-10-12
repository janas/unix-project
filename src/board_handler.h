/**
 * @file board_handler.h
 * @ingroup board_handler
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Aug 13, 2012
 *
 * @brief File containing methods for board manipulating and move checking.
 */

#ifndef BOARD_HANDLER_H_
#define BOARD_HANDLER_H_

#include "structs.h"

char** create_new_board(int size);
void destroy_board(char **board);
int get_board_size(char **board);
int make_move(char **board, move_s *move, int *free);

#endif /* BOARD_HANDLER_H_ */
