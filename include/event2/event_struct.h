/*
 * event_struct.h
 *
 *  Created on: Apr 20, 2017
 *      Author: magic
 */

#ifndef INCLUDE_EVENT2_EVENT_STRUCT_H_
#define INCLUDE_EVENT2_EVENT_STRUCT_H_

#include <sys/time.h>
#include <stdint.h>

#include "include/event2/util.h"
#include "include/event2/event.h"

#define EVLIST_TIMEOUT	    0x01
#define EVLIST_INSERTED	    0x02
#define EVLIST_SIGNAL	    0x04
#define EVLIST_ACTIVE	    0x08
#define EVLIST_INTERNAL	    0x10
#define EVLIST_ACTIVE_LATER 0x20
#define EVLIST_FINALIZING   0x40
#define EVLIST_INIT	        0x80

struct event_callback {
	struct {
		struct event_callback* tqe_next;
		struct event_callback* tqe_pre;
	}evcb_active_netx;
	short evcb_flags;
	uint8_t evcb_pri;
	uint8_t evcb_closure;

	union {
		void (*evcb_callback)(evutil_socket_t, short, void*);
		void (*evcb_selfcb)(struct event_callback*, void*);
		void (*evcb_evfinalize)(struct event *, void *);
		void (*evcb_cbfinalize)(struct event_callback *, void *);
	}evcb_cb_union;
	void* evcb_arg;
};

struct event_list {
	struct event* tqh_first;
	struct event** tqh_last;
};

struct event_base;
struct event {
	struct event_callback ev_evcallback;
	/* for managing timeouts */
	union {
		struct {
			struct event* tqe_next;
			struct event** tqe_prev;
		} ev_next_with_common_timeout;
		int min_heap_idx;
	} ev_timeout_pos;
	evutil_socket_t ev_fd;

	struct event_base* ev_base;

	union {
		struct {
			struct {
				struct event* le_next;
				struct event** le_prev;
			} ev_io_next;
			struct timeval ev_timeout;
		} ev_io;

		struct {
			struct {
				struct event* le_next;
				struct event** le_prev;
			} ev_signal_next;
			short ev_ncalls;
			/* Allows deletes in callback */
			short* ev_pncalls;
		} ev_signal;
	} ev_;

	short ev_events;
	/* result passed to event callback */
	short ev_res;
	struct timeval ev_timeout;
};

#endif /* INCLUDE_EVENT2_EVENT_STRUCT_H_ */
