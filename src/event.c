/*
 * event.c
 *
 *  Created on: Apr 20, 2017
 *      Author: magic
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "mm_internal.h"
#include "event_internal.h"
#include "include/event2/event.h"
#include "evthread_internal.h"

#define EVENT_BASE_ASSERT_LOCKED(base) EVLOCK_ASSERT_LOCKED((base)->th_base_lock)
/* How often (in seconds) do we check for changes in wall clock time relative
 * to monotonic time?  Set this to -1 for 'never.' */
#define CLOCK_SYNC_INTERVAL 5

struct event_base *event_global_current_base_ = NULL;
#define current_base event_global_current_base_

#define event_debug_assert_not_added_(ev) ((void)0)
#define event_debug_note_setup_(ev)       ((void)0)

static void *event_self_cbarg_ptr_ = NULL;

static void *(*mm_malloc_fn_)(size_t sz) = NULL;
static void *(*mm_realloc_fn_)(void *p, size_t sz) = NULL;
static void (*mm_free_fn_)(void *p) = NULL;

static int event_debug_mode_too_late = 1;

void *event_mm_malloc_(size_t sz)
{
	if (sz == 0)
		return NULL;

	if (mm_malloc_fn_)
		return mm_malloc_fn_(sz);
	else
		return malloc(sz);
}

void *event_mm_calloc_(size_t count, size_t size)
{
	if (count == 0 || size == 0)
		return NULL;

	if (mm_malloc_fn_) {
		size_t sz = count * size;
		void *p = NULL;
		if (count > EV_SIZE_MAX / size) {
			errno = ENOMEM;
			return NULL;
		}
		p = mm_malloc_fn_(sz);
		if (p)
			return memset(p, 0, sz);
	} else {
		void *p = calloc(count, size);
		return p;
	}
}

char* event_mm_strdup_(const char *str)
{
	if (!str) {
		errno = EINVAL;
		return NULL;
	}

	if (mm_malloc_fn_) {
		size_t ln = strlen(str);
		void *p = NULL;
		if (ln == EV_SIZE_MAX) {
			errno = ENOMEM;
			return NULL;
		}
		p = mm_malloc_fn_(ln+1);
		if (p)
			return memcpy(p, str, ln+1);
	} else {
		return strdup(str);
	}
}

void* event_mm_realloc_(void *ptr, size_t sz)
{
	if (mm_realloc_fn_)
		return mm_realloc_fn_(ptr, sz);
	else
		return realloc(ptr, sz);
}

void event_mm_free_(void *ptr)
{
	if (mm_free_fn_)
		mm_free_fn_(ptr);
	else
		free(ptr);
}

static const struct eventop* eventops[] = {

};

int event_priority_set(struct event *ev, int pri)
{
	event_debug_assert_is_setup_(ev);

	if (ev->ev_evcallback.evcb_flags & EVLIST_ACTIVE)
		return (-1);
	if (pri < 0 || pri >= ev->ev_base->nactivequeues)
		return (-1);

	ev->ev_evcallback.evcb_pri = pri;

	return (0);
}

int event_assign(struct event* ev, struct event_base* base, evutil_socket_t fd, short events,event_callback_fn callback, void* arg)
{
	if (!base) {
		base = current_base;
	}
	if (arg == &event_self_cbarg_ptr_) {
		arg = ev;
	}

	event_debug_assert_not_added_(ev);
	ev->ev_base = base;
	ev->ev_evcallback.evcb_cb_union.evcb_callback = callback;
	ev->ev_evcallback.evcb_arg = arg;
	ev->ev_fd = fd;
	ev->ev_events = events;
	ev->ev_res = 0;
	ev->ev_evcallback.evcb_flags = EVLIST_INIT;
	ev->ev_.ev_signal.ev_ncalls = 0;
	ev->ev_.ev_signal.ev_pncalls = NULL;

	if (events & EV_SIGNAL) {
		if ((events & (EV_READ|EV_WRITE|EV_CLOSED)) != 0) {
			event_log("%s: EV_SIGNAL is not compatible with "
				"EV_READ, EV_WRITE or EV_CLOSED", __FUNCTION__);
			return -1;
		}
		ev->ev_evcallback.evcb_closure = EV_CLOSURE_EVENT_SIGNAL;
	} else {
		if (events & EV_PERSIST) {
			evutil_timerclear(&ev->ev_.ev_io.ev_timeout);
			ev->ev_evcallback.evcb_closure = EV_CLOSURE_EVENT_PERSIST;
		} else {
			ev->ev_evcallback.evcb_closure = EV_CLOSURE_EVENT;
		}
	}

	min_heap_elem_init(ev);

	if (base != NULL) {
		ev->ev_evcallback.evcb_pri = base->nactivequeues / 2;
	}

	event_debug_note_setup_(ev);
	return 0;
}

static int gettime(struct event_base* base, struct timeval* tp)
{
	EVENT_BASE_ASSERT_LOCKED(base);

	if (base->tv_cache.tv_sec) {
		*tp = base->tv_cache;
		return 0;
	}

	if (evutil_gettime_monotonic_(&base->monotonic_timer, tp) == -1) {
		return -1;
	}

	if (base->last_updated_clock_diff + CLOCK_SYNC_INTERVAL < tp->tv_sec) {
		struct timeval tv;
		evutil_gettimeofday(&tv, NULL);
		evutil_timersub(&tv, tp, &base->tv_clock_diff);
		base->last_updated_clock_diff = tp->tv_sec;
	}

	return 0;
}

struct event_base* event_base_new(void)
{
	struct event_base* base = NULL;
	struct event_config* cfg = event_config_new();
	if (cfg) {
		base =
	}
}

struct event_base* event_base_new_with_config(const struct event_config* cfg)
{
	int i;
	struct event_base* base;
	int should_check_environment;
	event_debug_mode_too_late = 1;

	if ((base = mm_calloc(1, sizeof(struct event_base))) == NULL) {
		event_log("%s: calloc", __FUNCTION__);
		return NULL;
	}

	if (cfg) base->flags = cfg->flags;

	should_check_environment = !(cfg && (cfg->flags & EVENT_BASE_FLAG_IGNORE_ENV));
	{
		struct timeval tmp;
		int precise_time = cfg && (cfg->flags & EVENT_BASE_FLAG_PRECISE_TIMER);
		int flags;
		if (should_check_environment && !precise_time) {
			precise_time = (evutil_getenv_("EVENT_PRECISE_TIMER") != NULL);
			base->flags |= EVENT_BASE_FLAG_PRECISE_TIMER;
		}
		flags = precise_time ? EV_MONOT_PRECISE : 0;
		evutil_configure_monotonic_time_(&base->monotonic_timer, flags);

		gettime(base, &tmp);
	}

	min_heap_ctor_(&base->timeheap);

	base->sig.ev_signal_pair[0] = -1;
	base->sig.ev_signal_pair[1] = -1;
	base->th_notify_fd[0] = -1;
	base->th_notify_fd[1] = -1;

	base->active_later_quue.tqh_first = NULL;
	base->active_later_quue.tqh_last = &base->active_later_quue.tqh_first;

	evmap_io_initmap_(&base->io);
	evmap_signal_initmap_(&base->sigmap);

	base->evbase = NULL;

	if (cfg) {
		base->max_dispatch_time = cfg->max_dispatch_interval;
		base->limit_callbacks_after_prio = cfg->limit_callbacks_after_prio;
	} else {
		base->max_dispatch_time.tv_sec = -1;
		base->limit_callbacks_after_prio = 1;
	}

	if (cfg && cfg->max_dispathc_callbacks > 0) {
		base->max_dispatch_callbacks = cfg->max_dispathc_callbacks;
	} else {
		base->max_dispatch_callbacks = INT_MAX;
	}

	if (base->max_dispatch_callbacks == INT_MAX &&
		    base->max_dispatch_time.tv_sec == -1) {
		base->limit_callbacks_after_prio = INT_MAX;
	}

}

struct event_config* event_config_new(void)
{
	struct event_config* cfg = mm_calloc(1, sizeof(struct event_config));
	if (cfg == NULL) {
		return NULL;
	}

	cfg->entries.tqh_first = NULL;
	cfg->entries.tqh_last = &(cfg->entries.tqh_first);
	cfg->max_dispatch_interval = INT_MAX;
	cfg->limit_callbacks_after_prio = 1;

	return cfg;
}

void event_active_nolock_(struct event *ev, int res, short ncalls)
{

}





































