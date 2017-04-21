/*
 * util.h
 *
 *  Created on: Apr 20, 2017
 *      Author: magic
 */

#ifndef INCLUDE_EVENT2_UTIL_H_
#define INCLUDE_EVENT2_UTIL_H_

#define evutil_socket_t int
#define EV_SIZE_MAX sizeof(void*)
#define EV_SSIZE_MAX sizeof(void*)

/** Return true iff the tvp is related to uvp according to the relational
 * operator cmp.  Recognized values for cmp are ==, <=, <, >=, and >. */
#define	evutil_timercmp(tvp, uvp, cmp)					\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?				\
	 ((tvp)->tv_usec cmp (uvp)->tv_usec) :				\
	 ((tvp)->tv_sec cmp (uvp)->tv_sec))

#endif /* INCLUDE_EVENT2_UTIL_H_ */
