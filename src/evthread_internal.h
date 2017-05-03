/*
 * evthread_internal.h
 *
 *  Created on: Apr 24, 2017
 *      Author: magic
 */

#ifndef SRC_EVTHREAD_INTERNAL_H_
#define SRC_EVTHREAD_INTERNAL_H_

#include "include/event2/thread.h"

extern int evthread_lock_debugging_enabled_;

extern struct evthread_condition_callbacks evthread_cond_fns_;

int evthreadimpl_is_lock_debugging_enabled_(void);

int evthread_is_debug_lock_held_(void* lock);

unsigned long evthreadimpl_get_id_(void);

extern unsigned long (*evthread_id_fn_)(void);

#define EVLOCK_ASSERT_LOCKED(lock) \
	do {	\
		if ((lock) && evthreadimpl_is_lock_debugging_enabled_()) { \
			EVUTIL_ASSERT(evthread_is_debug_lock_held_(lock));	\
		}	\
	}while (0)

#define EVBASE_IN_THREAD(base) \
	(evthread_id_fn_ == NULL || \
	(base)->th_owner_id == evthread_id_fn_())

#define EVTHREAD_COND_WAIT(cond, lock)	\
	((cond) ? evthread_cond_fns_.wait_condition((cond), lock, NULL) : 0)

/** Return true iff we need to notify the base's main thread about changes to
 * its state, because it's currently running the main loop in another
 * thread. Requires lock. */
#define EVBASE_NEED_NOTIFY(base)			 \
	(evthread_id_fn_ != NULL &&			 \
	    (base)->running_loop &&			 \
	    (base)->th_owner_id != evthread_id_fn_())

#endif /* SRC_EVTHREAD_INTERNAL_H_ */




































