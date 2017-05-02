/*
 * thread.h
 *
 *  Created on: Apr 24, 2017
 *      Author: magic
 */

#ifndef INCLUDE_EVENT2_THREAD_H_
#define INCLUDE_EVENT2_THREAD_H_

void evthread_set_id_callback(unsigned long (*id_fn)(void));

struct evthread_condition_callbacks {
	int condition_api_version;

	void* (*alloc_condition)(unsigned condtype);

	void (*free_condition)(void* cond);

	int (*signal_condition)(void* cond, int broadcast);

	int (*wait_condition)(void* cond, void* lock, const struct timeval* timeout);
};

#endif /* INCLUDE_EVENT2_THREAD_H_ */
