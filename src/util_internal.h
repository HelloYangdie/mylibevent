/*
 * util_internal.h
 *
 *  Created on: Apr 24, 2017
 *      Author: magic
 */

#ifndef SRC_UTIL_INTERNAL_H_
#define SRC_UTIL_INTERNAL_H_

#include "log_internal.h"

/* Structure to hold the state of our weak random number generator.
 */
struct evutil_weakrand_state {
	unsigned int seed;
};

#if defined(__GNUC__) && __GNUC__ >= 3         /* gcc 3.0 or later */
#define EVUTIL_UNLIKELY(p) __builtin_expect(!!(p),0)
#else
#define EVUTIL_UNLIKELY(p) (p)
#endif

#define EVUTIL_ASSERT(cond)	\
	do {	\
		if (EVUTIL_UNLIKELY(!(cond))) {	\
			event_log("%s:%d: Assertion %s failed in %s",__FILE__,__LINE__,#cond,__FUNCTION__);\
			abort();	\
		}	\
	}while (0)


const char *evutil_getenv_(const char *name);

#endif /* SRC_UTIL_INTERNAL_H_ */





























