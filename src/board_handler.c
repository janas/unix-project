/**
 * @file board_handler.c
 * @ingroup board_handler
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Aug 13, 2012
 *
 * @brief File containing methods for board manipulating and move checking.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "structs.h"

/**
 * Initializes a board by setting all fields to '0'.
 * @param[in] board Pointer to the board to be initialized.
 */
void
initialize_board(char **board) {
	int i, j;
	for (i = 0; i < NROWS; i++)
		for (j = 0; j < NCOLS; j++)
			board[i][j] = '0';
}

/**
 * Prepares a board by setting fields to '1' on a given size.
 * @param[in] board Pointer to the board to be prepared.
 * @param[in] size  Size of the board to be prepared.
 */
void
prepare_board(char **board, int size) {
	int i, j;
	for (i = 0; i < size; i++)
		for (j = 0; j < size; j++)
			board[i][j] = '1';
}

/**
 * Creates new board of a given size.
 * @param[in] size The size of the board to be created.
 * @return Pointer to the created board.
 */
char**
create_new_board(int size) {
	int i;
	char **board = (char**) malloc(NROWS * sizeof(char*));
	if (board == NULL) {
		return NULL;
	}
	for (i = 0; i < NROWS; i++) {
		board[i] = (char*) malloc(NCOLS * sizeof(char));
		if (board[i] == NULL) {
			return NULL;
		}
	}
	initialize_board(board);
	prepare_board(board, size);
	return board;
}

/**
 * Destroys a board by freeing all the pointers.
 * @param[in] board Pointer to the board to be destroyed.
 */
void
destroy_board(char **board) {
	int i;
	for (i = 0; i < NROWS; i++)
		free(board[i]);
	free(board);
}

/**
 * Checks the size of a given board.
 * @param[in] board Pointer to the board which size needs to be checked.
 * @return The size of the board i.e. the value for which the board was prepared.
 * \sa prepare_board
 */
int
get_board_size(char **board) {
	int i, j, rows, cols;
	rows = cols = 0;
	if (board == NULL) {
		return -1;
	}
	for (i = 0; i < NROWS; i++) {
		if (board[i][0] == '1' || board[i][0] == 'x' || board[i][0] == 'o') {
			rows++;
		}
	}
	for (j = 0; j < NCOLS; j++) {
		if (board[0][j] == '1' || board[0][j] == 'x' || board[0][j] == 'o') {
			cols++;
		}
	}
	if (rows == cols) {
		return rows;
	}
	return -1;
}

/**
 * Validates whether a specified move can be performed on a given board.
 * @param[in] board Pointer to the board that a move will be performed on.
 * @param[in] move  Structure containing coordinates of a move and a player's pawn.
 * @retval  0 When a move can be performed i.e. the field is empty.
 * @retval -1 When an error occurs i.e. the field is not empty or is out of current range.
 * \sa move_s
 */
int
validate_move(char **board, move_s *move) {
	if (board[move->x][move->y] == 'x') {
		return -1;
	}
	if (board[move->x][move->y] == 'o') {
		return -1;
	}
	if (board[move->x][move->y] == '1') {
		return 0;
	}
	if (board[move->x][move->y] == '0') {
		return -1;
	}
	return -1;
}

/**
 * Checks whether a four consecutive pawns can be found in any horizontal row.
 * Then the game wins the player that makes current move.
 * @param[in] board Pointer to the board that a will be check against four consecutive pawns in a row.
 * @param[in] size  Size of the board to be checked.
 * @param[in] pawn  Pawn of a player that currently makes a move.
 * @retval  0 When a four consecutive pawns are find in a row.
 * @retval -1 When a four consecutive pawns are not found.
 */
int
check_board_horizontally(char **board, int size, char pawn) {
	int row, col, count = 0;
	for (row = 0; row < size; row++) {
		for (col = 0; col < size; col++) {
			if (board[row][col] == pawn) {
				count++;
				if (count == 4) {
					return 0;
				}
			} else {
				count = 0;
			}
		}
		count = 0;
	}
	return -1;
}

/**
 * Checks whether a four consecutive pawns can be found in any vertical row.
 * Then the game wins the player that makes current move.
 * @param[in] board Pointer to the board that a will be check against four consecutive pawns in a row.
 * @param[in] size  Size of the board to be checked.
 * @param[in] pawn  Pawn of a player that currently makes a move.
 * @retval  0 When a four consecutive pawns are find in a row.
 * @retval -1 When a four consecutive pawns are not found.
 */
int
check_board_vertically(char **board, int size, char pawn) {
	int row, col, count = 0;
	for (col = 0; col < size; col++) {
		for (row = 0; row < size; row++) {
			if (board[row][col] == pawn) {
				count++;
				if (count == 4) {
					return 0;
				}
			} else {
				count = 0;
			}
		}
		count = 0;
	}
	return -1;
}

/**
 * Checks whether a four consecutive pawns can be found in any skew right row in the lower part of a board.
 * Then the game wins the player that makes current move.
 * @param[in] board Pointer to the board that a will be check against four consecutive pawns in a row.
 * @param[in] size  Size of the board to be checked.
 * @param[in] pawn  Pawn of a player that currently makes a move.
 * @retval  0 When a four consecutive pawns are find in a row.
 * @retval -1 When a four consecutive pawns are not found.
 */
int
check_board_skew_right_down(char **board, int size, char pawn) {
	int row, col, tmp_row, count = 0;
	for (row = 0; row < size; row++) {
		tmp_row = row;
		for (col = 0; col < size && tmp_row < size; col++, tmp_row++) {
			if (board[tmp_row][col] == pawn) {
				count++;
				if (count == 4) {
					return 0;
				}
			} else {
				count = 0;
			}
		}
		count = 0;
	}
	return -1;
}

/**
 * Checks whether a four consecutive pawns can be found in any skew right row in the upper part of a board.
 * Then the game wins the player that makes current move.
 * @param[in] board Pointer to the board that a will be check against four consecutive pawns in a row.
 * @param[in] size  Size of the board to be checked.
 * @param[in] pawn  Pawn of a player that currently makes a move.
 * @retval  0 When a four consecutive pawns are find in a row.
 * @retval -1 When a four consecutive pawns are not found.
 */
int
check_board_skew_right_up(char **board, int size, char pawn) {
	int row, col, tmp_col, count = 0;
	for (col = 0; col < size; col++) {
		tmp_col = col;
		for (row = 0; row < size && tmp_col < size; row++, tmp_col++) {
			if (board[row][tmp_col] == pawn) {
				count++;
				if (count == 4) {
					return 0;
				}
			} else {
				count = 0;
			}
		}
		count = 0;
	}
	return -1;
}

/**
 * Checks whether a four consecutive pawns can be found in any skew left row in the lower part of a board.
 * Then the game wins the player that makes current move.
 * @param[in] board Pointer to the board that a will be check against four consecutive pawns in a row.
 * @param[in] size  Size of the board to be checked.
 * @param[in] pawn  Pawn of a player that currently makes a move.
 * @retval  0 When a four consecutive pawns are find in a row.
 * @retval -1 When a four consecutive pawns are not found.
 */
int
check_board_skew_left_down(char **board, int size, char pawn) {
	int row, col, tmp_col, count = 0;
	for (col = 0; col < size; col++) {
		tmp_col = col;
		for (row = size - 1; row >= 0 && tmp_col < size; row--, tmp_col++) {
			if (board[row][tmp_col] == pawn) {
				count++;
				if (count == 4) {
					return 0;
				}
			} else {
				count = 0;
			}
		}
		count = 0;
	}
	return -1;
}

/**
 * Checks whether a four consecutive pawns can be found in any skew left row in the upper part of a board.
 * Then the game wins the player that makes current move.
 * @param[in] board Pointer to the board that a will be check against four consecutive pawns in a row.
 * @param[in] size  Size of the board to be checked.
 * @param[in] pawn  Pawn of a player that currently makes a move.
 * @retval  0 When a four consecutive pawns are find in a row.
 * @retval -1 When a four consecutive pawns are not found.
 */
int
check_board_skew_left_up(char **board, int size, char pawn) {
	int row, col, tmp_row, count = 0;
	for (row = size - 1; row >= 0; row--) {
		tmp_row = row;
		for (col = 0; col < size && tmp_row >= 0; col++, tmp_row--) {
			if (board[tmp_row][col] == pawn) {
				count++;
				if (count == 4) {
					return 0;
				}
			} else {
				count = 0;
			}
		}
		count = 0;
	}
	return -1;
}

/**
 * Method coupling several function responsible for checking, validating and performing a given move.
 * @param[in] board Pointer to the board that a move will be performed on.
 * @param[in] move  Structure containing coordinates of a move and a player's pawn.
 * @param[in] free  Pointer showing how many free places left on the board.
 * @retval  0 When a move is performed and a game is not finished.
 * @retval  1 When a move is performed and current player wins the game.
 * @retval -1 When current move cannot be performed i.e. is out of range.
 * \sa move_s
 */
int
make_move(char **board, move_s *move, int *free) {
	int size, result = 0;
	if ((size = get_board_size(board)) == -1) {
		return -1;
	}
	if (move->x > size || move->y > size || move->x < 0 || move->y < 0) {
		return -1;
	}
	if (validate_move(board, move) != 0) {
		return -1;
	}
	board[move->x][move->y] = move->pawn;
	(*free)--;
	if (*free <= 0) {
		return 2;
	}
	if ((result = check_board_horizontally(board, size, move->pawn)) == 0) {
		return 1;
	}
	if ((result = check_board_vertically(board, size, move->pawn)) == 0) {
		return 1;
	}
	if ((result = check_board_skew_right_down(board, size, move->pawn)) == 0) {
		return 1;
	}
	if ((result = check_board_skew_right_up(board, size, move->pawn)) == 0) {
		return 1;
	}
	if ((result = check_board_skew_left_down(board, size, move->pawn)) == 0) {
		return 1;
	}
	if ((result = check_board_skew_left_up(board, size, move->pawn)) == 0) {
		return 1;
	}
	return 0;
}
