/*
 * evsignal_internal.h
 *
 *  Created on: Apr 21, 2017
 *      Author: magic
 */

#ifndef SRC_EVSIGNAL_INTERNAL_H_
#define SRC_EVSIGNAL_INTERNAL_H_

#include <signal.h>
#include "include/event2/util.h"

typedef void (*ev_sighandler_t)(int);

struct evsig_info {
	struct event ev_signal;
	evutil_socket_t ev_signal_pair[2];
	int ev_signal_added;
	int ev_n_signals_added;

	struct sigaction** sh_old;
	int sh_old_max;
};

int evsig_init_(struct event_base *);
void evsig_dealloc_(struct event_base *);

void evsig_set_base_(struct event_base *base);
void evsig_free_globals_(void);

#endif /* SRC_EVSIGNAL_INTERNAL_H_ */
