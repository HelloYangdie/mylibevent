/*
 * util.h
 *
 *  Created on: Apr 20, 2017
 *      Author: magic
 */

#ifndef INCLUDE_EVENT2_UTIL_H_
#define INCLUDE_EVENT2_UTIL_H_

#include <stdint.h>
#include <sys/time.h>

#define EVENT_SIZEOF_SIZE_T  sizeof(void*)

#define evutil_socket_t int

#define EV_SIZE_MAX UINT64_MAX

#if EVENT_SIZEOF_SIZE_T == 4
#define EV_SIZE_MAX UINT32_MAX
#define EV_SSIZE_MAX UINT32_MAX
#elif EVENT_SIZEOF_SIZE_T == 8
#define EV_SIZE_MAX UINT64_MAX
#define EV_SSIZE_MAX UINT64_MAX
#endif

/** Return true iff the tvp is related to uvp according to the relational
 * operator cmp.  Recognized values for cmp are ==, <=, <, >=, and >. */
#define	evutil_timercmp(tvp, uvp, cmp)					\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?				\
	 ((tvp)->tv_usec cmp (uvp)->tv_usec) :				\
	 ((tvp)->tv_sec cmp (uvp)->tv_sec))

#define EV_MONOT_PRECISE  1
#define EV_MONOT_FALLBACK 2

#define evutil_gettimeofday(tv, tz) gettimeofday((tv), (tz))
#define evutil_timeradd(tvp, uvp, vvp) timeradd((tvp),(uvp),(vvp))
#define evutil_timersub(tvp, uvp, vvp) timersub((tvp),(uvp),(vvp))

/** Do platform-specific operations as needed to close a socket upon a
    successful execution of one of the exec*() functions.

    @param sock The socket to be closed
    @return 0 on success, -1 on failure
 */
int evutil_make_socket_closeonexec(evutil_socket_t sock);

int evutil_make_socket_nonblocking(evutil_socket_t sock);


#endif /* INCLUDE_EVENT2_UTIL_H_ */
































