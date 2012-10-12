/**
 * @file mainpage.h
 * @ingroup mainpage
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Jul 6, 2012
 *
 * @brief File containing Main Page of the documentation.
 */

#ifndef MAINPAGE_H_
#define MAINPAGE_H_

/** @mainpage Four-in-a-line game
*
* @authors Piotr Janaszek <janas03@yahoo.pl>
*
* @section intro Introduction
* This is one of classic pen-and-paper games. This task is to implement a client application
* and a server hosting any number of games. It is a server's responsibility to pair clients and
* check when the game ends. For each pair of clients, the server creates a separate thread. All
* communication is realized by TCP sockets. After connecting to the server, a client can list
* existing games - both paired and unpaired, player nicks and game board size. Then, the
* connected client can choose to watch an existing game, connect to other unpaired client and
* start the game or create his own game. Clients are not paired automatically. When a client
* creates a game it chooses a nick and game board size (varying from 4 to 20). When a client is
* connected, a board is displayed with the current game state. During the game, clients in a
* pair can chat privately with each other. After the game client is not disconnected, it can
* start all over again.
*
*
* Also note the existence of the following directories:
* - src
* -# Contains the source code files of the game
* - doc
* -# Contains the documentation files (if doxygen documentation is generated)
*
*
*
* <hr>
* @section notes Release notes
* This is final version 1.0. If there is a bug that you have found please contact the developer
*  at <janas03@yahoo.pl>.
* <hr>
* @section requirements Requirements
* <hr>
* In order to build this package you will need standard GCC compiler with pthreads library.
*
*/

#endif /* MAINPAGE_H_ */
