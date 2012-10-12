/**
 * @file config.h
 * @ingroup config
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 6, 2012
 *
 * @brief File containing configuration values used upon building.
 */

#ifndef CONFIG_H_
#define CONFIG_H_

/*! \def ERR(source)
 * Macro for printing an error and its source.
 */
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

/**
 * Number of connected clients at a time.
 */
#define BACKLOG 10

/**
 * Size of the header buffer.
 */
#define HEADER 4

/**
 * Size of the error buffer.
 */
#define ERROR 4

/**
 * Maximum number of spectators connected to a game.
 */
#define SPECTATORS_NO 5

/**
 * Message delimiter. It is used to separate header (and error) from payload.
 */
#define MSG_DELIM "\r\n\r\n"

/**
 * Payload delimiter.
 */
#define PAYLOAD_DELIM "#"

/**
 * Delimiter of a given string contained between payload delimiter.
 * \sa PAYLOAD_DELIM
 */
#define INNER_DELIM ";"

/**
 * Name of the fifo file created while server is running.
 */
#define FIFO_NAME "temp"

/**
 * Size of the board - rows.
 */
#define NROWS 20

/**
 * Size of the board - columns.
 */
#define NCOLS 20

/**
 * Restriction set while creating a new game.
 */
#define MAX_BOARD_SIZE 20

/**
 * Restriction set while creating a new game.
 */
#define MIN_BOARD_SIZE 4

/**
 * Maximum length of a message.
 */
#define MAX_MSG_SIZE 512

/**
 * Maximum length of a request payload.
 */
#define MAX_REQ_SIZE 512 - HEADER

/**
 * Maximum length of a response payload.
 */
#define MAX_RSP_SIZE 512 - HEADER - ERROR

/**
 * Maximum nick length.
 */
#define MAX_NICK_LEN 32

#endif /* CONFIG_H_ */
