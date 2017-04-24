/*
 * evutil_time.c
 *
 *  Created on: Apr 24, 2017
 *      Author: magic
 */

#include "time_internal.h"
#include "include/event2/util.h"
#include "util_internal.h"

static void adjust_monotonic_time(struct evutil_monotonic_timer* base, struct timeval* tv)
{

}

int evutil_configure_monotonic_time_(struct evutil_monotonic_timer *base, int flags)
{
#ifdef CLOCK_MONOTONIC_COARSE
	const int precise = flags & EV_MONOT_PRECISE;
#endif
	const int fallback = flags & EV_MONOT_FALLBACK;
	struct timespec ts;

#ifdef CLOCK_MONOTONIC_COARSE /*very fast, but not fine-grained timestamps*/
	if (CLOCK_MONOTONIC_COARSE < 0) {
		event_log("I didn't expect CLOCK_MONOTONIC_COARSE to be < 0");
	}
	if (!precise && !fallback) {
		if (clock_gettime(CLOCK_MONOTONIC_COARSE, &ts) == 0) {
			base->monotonic_clock = CLOCK_MONOTONIC_COARSE;
			return 0;
		}
	}
#endif
	if (!fallback && clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		base->monotonic_clock = CLOCK_MONOTONIC;
		return 0;
	}

	base->monotonic_clock = -1;
	return 0;
}

int evutil_gettime_monotonic_(struct evutil_monotonic_timer* base, struct timeval* tp)
{
	struct timespec ts;

	if (base->monotonic_clock < 0) {
		if (evutil_gettimeofday(tp, NULL) < 0) return -1;
		adjust_monotonic_time(base, tp);
		return 0;
	}

	if (clock_gettime(base->monotonic_clock, &ts) == -1)
		return -1;
	tp->tv_sec = ts.tv_sec;
	tp->tv_usec = ts.tv_nsec / 1000;

	return 0;
}
































