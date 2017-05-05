/*
 * event_internal.h
 *
 *  Created on: Apr 20, 2017
 *      Author: magic
 */

#ifndef SRC_EVENT_INTERNAL_H_
#define SRC_EVENT_INTERNAL_H_

#include <time.h>

#include "include/event2/event_struct.h"
#include "evsignal_internal.h"
#include "time_internal.h"
#include "util_internal.h"
#include "minheap_internal.h"
#include "queue_internal.h"

/** @name Event closure codes
    Possible values for evcb_closure in struct event_callback
**/

/** A regular event. Uses the evcb_callback callback */
#define EV_CLOSURE_EVENT 0
/** A signal event. Uses the evcb_callback callback */
#define EV_CLOSURE_EVENT_SIGNAL 1
/** A persistent non-signal event. Uses the evcb_callback callback */
#define EV_CLOSURE_EVENT_PERSIST 2
/** A simple callback. Uses the evcb_selfcb callback. */
#define EV_CLOSURE_CB_SELF 3
/** A finalizing callback. Uses the evcb_cbfinalize callback. */
#define EV_CLOSURE_CB_FINALIZE 4
/** A finalizing event. Uses the evcb_evfinalize callback. */
#define EV_CLOSURE_EVENT_FINALIZE 5
/** A finalizing event that should get freed after. Uses the evcb_evfinalize
 * callback. */
#define EV_CLOSURE_EVENT_FINALIZE_FREE 6


#define event_io_map event_signal_map

struct event_config_entry {
	struct {
		struct event_config_entry* tqe_next;
		struct event_config_entry** tqe_prev;
	}next;

	const char* avoid_method;
};

struct event_signal_map {
	void** entries;
	int nentries;
};
/** Internal structure: describes the configuration we want for an event_base
 * that we're about to allocate. */
struct event_config {
	struct event_configq {
		struct event_config_entry* tqh_first;
		struct event_config_entry** tqh_last;
	} entries;

	int n_cpus_hint;
	struct timeval max_dispatch_interval;
	int max_dispathc_callbacks;
	int limit_callbacks_after_prio;
	enum event_method_feature require_features;
	enum event_base_config_flag flags;
};

/* List of 'changes' since the last call to eventop.dispatch.  Only maintained
 * if the backend is using changesets. */
struct event_changelist {
	struct event_change *changes;
	int n_changes;
	int changes_size;
};

struct evcallback_list {
	struct event_callback* tqh_first;
	struct event_callback** tqh_last;
};

struct eventop {
	const char* name;
	void* (*init)(struct event_base*);
	int (*add)(struct event_base*, evutil_socket_t fd, short old, short events, void* fdinfo);
	int (*del)(struct event_base*, evutil_socket_t fd, short old, short events, void* fdinfo);
	void (*dispatch)(struct event_base*, struct timeval*);
	void (*dealloc)(struct event_base*);

	/*Flag: set if we need to reinitialize the event base after we fork*/
	int need_reinit;

	enum event_method_feature features;
	size_t fdinfo_len;
};

struct common_timeout_list {
	/* List of events currently waiting in the queue. */
	struct event_list events;
	/* 'magic' timeval used to indicate the duration of events in this queue. */
	struct timeval duratin;
	/* Event that triggers whenever one of the events in the queue is ready to activate */
	struct event timeout_event;
	/* The event_base that this timeout list is part of */
	struct event_base* base;
};

struct event_base {
	/** Function pointers and other data to describe this event_base's backend. */
	const struct eventop* evsel;
	/** Pointer to backend-specific data. */
	void* evbase;
	/** List of changes to tell backend about at next dispatch.  Only used
	 * by the O(1) backends. */
	struct event_changelist changelist;
	/** Function pointers used to describe the backend that this event_base
	 * uses for signals */
	const struct eventop* evsigsel;

	/** Data to implement the common signal handelr code. */
	struct evsig_info sig;

	int virtual_event_count;
	int virtual_event_count_max;

	int event_count;
	int event_count_max;
	int event_count_active;
	int event_count_active_max;

	/** Set if we should terminate the loop once we're done processing events. */
	int event_gotterm;
	/** Set if we should terminate the loop immediately */
	int event_break;
	/** Set if we should start a new instance of the loop immediately. */
	int event_continue;
	/** The currently running priority of events */
	int event_running_priority;
	/** Set if we're running the event_base_loop function, to prevent
	 * reentrant invocation. */
	int running_loop;

	/** Set to the number of deferred_cbs we've made 'active' in the
	 * loop.  This is a hack to prevent starvation; it would be smarter
	 * to just use event_config_set_max_dispatch_interval's max_callbacks
	 * feature */
	int n_deferreds_queued;
	/* Active event management. */
	/** An array of nactivequeues queues for active event_callbacks (ones
	 * that have triggered, and whose callbacks need to be called).  Low
	 * priority numbers are more important, and stall higher ones.
	 */
	struct evcallback_list* activequeues;
	/** The length of the activequeues array */
	int nactivequeues;

	struct evcallback_list active_later_queue;

	/** An array of common_timeout_list* for all of the common timeout values we know. */
	struct common_timeout_list** common_timeout_queues;
	int n_common_timeouts;
	/** The total size of common_timeout_queues. */
	int n_common_timeouts_allocated;

	struct event_io_map io;

	struct event_signal_map sigmap;

	/** Priority queue of events with timeouts. */
	struct min_heap timeheap;

	struct timeval tv_cache;

	struct evutil_monotonic_timer monotonic_timer;

	/** Difference between internal time (maybe from clock_gettime) and gettimeofday. */
	struct timeval tv_clock_diff;

	/** Second in which we last updated tv_clock_diff, in monotonic time. */
	time_t last_updated_clock_diff;

	/** The thread id currently running the event_loop for this base */
	unsigned long th_owner_id;

	void* th_base_lock;
	/** A condition that gets signalled when we're done processing an
	 * event with waiters on it. */
	void* current_event_cond;
	/** Number of threads blocking on current_event_cond. */
	int current_event_waiters;
	/** The event whose callback is executing right now */
	struct event_callback* current_event;

	/** Flags that this base was configured with */
	enum event_base_config_flag flags;

	struct timeval max_dispatch_time;
	int max_dispatch_callbacks;
	int limit_callbacks_after_prio;
	/** True if the base already has a pending notify, and we don't need
	 * to add any more. */
	int is_notify_pending;
	/** A socketpair used by some th_notify functions to wake up the main
	 * thread. */
	evutil_socket_t th_notify_fd[2];
	/** An event used by some th_notify functions to wake up the main
	 * thread. */
	struct event th_notify;
	/** A function used to wake up the main thread from another thread. */
	int (*th_notify_fn)(struct event_base* base);

	struct evutil_weakrand_state weakrand_seed;

	struct once_event_list {
		struct event_once* lh_first;
	} once_events;
};

void event_active_nolock_(struct event *ev, int res, short count);

int event_callback_active_nolock_(struct event_base*, struct event_callback*);

int event_callback_activate_nolock_(struct event_base *, struct event_callback *);

#endif /* SRC_EVENT_INTERNAL_H_ */







































