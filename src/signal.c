/*
 * signal.c
 *
 *  Created on: Apr 25, 2017
 *      Author: magic
 */

#include "evsignal_internal.h"
#include "mm_internal.h"
#include "include/event2/event.h"
#include "include/event2/event_struct.h"

static const struct eventop evsigops = {

};

static void evsig_cb(evutil_socket_t fd, short what, void *arg)
{

}

int evsig_init(struct event_base* base)
{
	if (evutil_make_internal_pipe_(base->sig.ev_signal_pair) == -1) {
		event_log("%s socketpair", __FUNCTION__);
		return -1;
	}

	if (base->sig.sh_old) {
		mm_free(base->sig.sh_old);
	}
	base->sig.sh_old = NULL;
	base->sig.sh_old_max = 0;

	event_assign(&base->sig.ev_signal, base, base->sig.ev_signal_pair[0],
			EV_READ | EV_PERSIST, evsig_cb, base);

	base->sig.ev_signal.ev_evcallback.evcb_flags |= EVLIST_INTERNAL;

	event_priority_set(&base->sig.ev_signal, 0);
	base->evsigsel = &evsigops;

	return 0;
}

































