/*
 * evthread.c
 *
 *  Created on: Apr 24, 2017
 *      Author: magic
 */

#include <stdlib.h>

int evthread_lock_debugging_enabled_ = 0;

unsigned long (*evthread_id_fn_)(void) = NULL;

struct debug_lock {
	unsigned signature;
	unsigned locktype;
	unsigned long held_by;
	int count;
	void* lock;
};

struct evthread_condition_callbacks evthread_cond_fns_ = {
	0, NULL, NULL, NULL, NULL
};

struct evthread_lock_callbacks evthread_lock_fns_ = {
		0, 0, NULL, NULL, NULL, NULL
};

int evthreadimpl_is_lock_debugging_enabled_(void)
{
	return evthreadimpl_is_lock_debugging_enabled_;
}

int evthread_is_debug_lock_held_(void* lock_)
{
	struct debug_lock* lock = lock_;
	if (!lock->count) return 0;
	if (evthread_id_fn_) {
		unsigned long me = evthread_id_fn_();
		if (lock->held_by != me) return 0;
	}

	return 1;
}

void evthread_set_id_callback(unsigned long (*id_fn)(void))
{
	evthread_id_fn_ = id_fn;
}
































