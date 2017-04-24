/*
 * evthread_internal.h
 *
 *  Created on: Apr 24, 2017
 *      Author: magic
 */

#ifndef SRC_EVTHREAD_INTERNAL_H_
#define SRC_EVTHREAD_INTERNAL_H_

extern int evthread_lock_debugging_enabled_;

int evthreadimpl_is_lock_debugging_enabled_(void);

int evthread_is_debug_lock_held_(void* lock);

extern unsigned long (*evthread_id_fn_)(void);

#define EVLOCK_ASSERT_LOCKED(lock) \
	do {	\
		if ((lock) && evthreadimpl_is_lock_debugging_enabled_()) { \
			EVUTIL_ASSERT(evthread_is_debug_lock_held_(lock));	\
		}	\
	}while (0)


#endif /* SRC_EVTHREAD_INTERNAL_H_ */
