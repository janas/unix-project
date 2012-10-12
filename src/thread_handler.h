/**
 * @file thread_handler.h
 * @ingroup thread_handler
 *
 * @author Piotr Janaszek <janas03@yahoo.pl>
 * @date Created on: Aug 14, 2012
 *
 * @brief File containing methods for handling requests
 * received from clients (during the game).
 */

#ifndef THREAD_HANDLER_H_
#define THREAD_HANDLER_H_

void initialize_thread(thread_data_s *targs, threads_list_s *threads_list,
		pthread_mutex_t *threads_list_mutex);

#endif /* THREAD_HANDLER_H_ */
