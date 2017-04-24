/*
 * time_internal.h
 *
 *  Created on: Apr 24, 2017
 *      Author: magic
 */

#ifndef SRC_TIME_INTERNAL_H_
#define SRC_TIME_INTERNAL_H_

#include <time.h>

struct evutil_monotonic_timer {
	int monotonic_clock;
	struct timeval adjust_monotonic_clock;
	struct timeval last_time;
};

int evutil_configure_monotonic_time_(struct evutil_monotonic_timer *mt, int flags);
int evutil_gettime_monotonic_(struct evutil_monotonic_timer *mt, struct timeval *tv);

#endif /* SRC_TIME_INTERNAL_H_ */
