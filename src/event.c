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
#include "queue_internal.h"

#define EVENT_BASE_ASSERT_LOCKED(base) EVLOCK_ASSERT_LOCKED((base)->th_base_lock)
/* How often (in seconds) do we check for changes in wall clock time relative
 * to monotonic time?  Set this to -1 for 'never.' */
#define CLOCK_SYNC_INTERVAL 5

struct event_base *event_global_current_base_ = NULL;
#define current_base event_global_current_base_

#define event_debug_assert_not_added_(ev) ((void)0)
#define event_debug_note_setup_(ev)       ((void)0)


#define MAX(a,b) (((a)>(b))?(a):(b))
#define MAX_EVENT_COUNT(var, v) var = MAX(var, v)

#define DECR_EVENT_COUNT(base,flags) \
	((base)->event_count -= (~((flags) >> 4) & 1))

#define INCR_EVENT_COUNT(base,flags) do {					\
	((base)->event_count += (~((flags) >> 4) & 1));				\
	MAX_EVENT_COUNT((base)->event_count_max, (base)->event_count);		\
} while (0)

static void *event_self_cbarg_ptr_ = NULL;

static void *(*mm_malloc_fn_)(size_t sz) = NULL;
static void *(*mm_realloc_fn_)(void *p, size_t sz) = NULL;
static void (*mm_free_fn_)(void *p) = NULL;

static int event_debug_mode_too_late = 1;

extern const struct eventop epollops;

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
		epollops,
		NULL
};

static void event_base_free_(struct event_base* base, int run_finalizers)
{

}

void event_base_free(struct event_base *base)
{
	event_base_free_(base, 1);
}

static struct event_callback* event_to_event_callback(struct event* ev)
{
	return &ev->ev_evcallback;
}

static int evthread_notify_base(struct event_base* base)
{
	if (!base->th_notify_fn) return -1;

	if (base->is_notify_pending) return 0;

	base->is_notify_pending = 1;
	return base->th_notify_fn(base);
}

static int evthread_notify_base_eventfd(struct event_base* base)
{
	uint64_t msg = 1;
	int r = 0;

	do {
		r = write(base->th_notify_fd[0], (void* )msg, sizeof(msg));
	} while (r < 0 && errno == EAGAIN);

	return r < 0 ? -1 : 0;
}

static void evthread_notify_drain_eventfd(evutil_socket_t fd, short what, void* arg)
{
	uint64_t msg;
	ssize_t r;
	struct event_base* base = arg;

	r = read(fd, (void*)&msg, sizeof(msg));
	if (r < 0 && errno != EAGAIN) {
		event_log(""Error reading from eventfd");
	}

	EVBASE_ACQUIRE_LOCK(base, th_base_lock);
	base->is_notify_pending = 0;
	EVBASE_RELEASE_LOCK(base, th_base_lock);
}

static void event_queue_remove_active_later(struct event_base* base, struct event_callback* evcb)
{
	if (EVUTIL_UNLIKELY(!(evcb->evcb_flags & EVLIST_ACTIVE_LATER))) {
		event_log("%s: %p not on queue %x", __FUNCTION__, evcb, EVLIST_ACTIVE_LATER);
		return;
	}
	DECR_EVENT_COUNT(base, evcb->evcb_flags);
	evcb->evcb_flags &= ~EVLIST_ACTIVE_LATER;
	base->event_count_active--;

	TAILQ_REMOVE(&base->active_later_queue, evcb, evcb_active_next);
}

static void event_queue_insert_active(struct event_base* base, struct event_callback* evcb)
{
	if (evcb->evcb_flags & EVLIST_ACTIVE) {
		return;
	}

	INCR_EVENT_COUNT(base, evcb->evcb_flags);

	evcb->evcb_flags |= EVLIST_ACTIVE;
	base->event_count_active++;
	MAX_EVENT_COUNT(base->event_count_active_max, base->event_count_active);
	TAILQ_INSERT_TAIL(&base->activequeues[evcb->evcb_pri], evcb, evcb_active_next);
}

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

static void event_config_entry_free(struct event_config_entry *entry)
{
	if (entry->avoid_method != NULL)
		mm_free((char *)entry->avoid_method);
	mm_free(entry);
}

void event_config_free(struct event_config* cfg)
{
	struct event_config_entry* entry;
	while ((entry = TAILQ_FIRST(&cfg->entries)) != NULL) {
		TAILQ_REMOVE(&cfg->entries, entry, next);
		event_config_entry_free(entry);
	}
	mm_free(cfg);
}

struct event_base* event_base_new(void)
{
	struct event_base* base = NULL;
	struct event_config* cfg = event_config_new();
	if (cfg) {
		base = event_base_new_with_config(cfg);
		event_config_free(cfg);
	}
}

static int event_config_is_avoided_method(const struct event_config* cfg, const char* method)
{
	struct event_config_entry* entry;
	TAILQ_FOREACH(entry, &cfg->entries, next) {
		if (entry->avoid_method != NULL &&
			strcmp(entry->avoid_method, method) == 0) {
			return 1;
		}
	}

	return 0;
}

static int evthread_make_base_notifiable_nolock_(struct event_base *base)
{
	void (*cb)(evutil_socket_t, short, void*);
	int (*notify)(struct event_base*);

	if (base->th_notify_fn != NULL) return 0;

	base->th_notify_fd[0] = evutil_eventfd_(0, EVUTIL_EFD_CLOEXEC|EVUTIL_EFD_NONBLOCK);
	if (base->th_notify_fd[0] >= 0) {
		base->th_notify_fd[1] = -1;
		notify = evthread_notify_base_eventfd;
		cb = evthread_notify_drain_eventfd;
	}
}

int event_base_priority_init(struct event_base* base, int npriorities)
{
	int i;

	if (base->event_count_active || npriorities < 1 || npriorities >= EVENT_MAX_PRIORITIES) {
		return -1;
	}

	if (npriorities == base->nactivequeues) return -1;

	if (base->nactivequeues) {
		mm_free(base->activequeues);
		base->nactivequeues = 0;
	}

	base->activequeues = (struct evcallback_list* )mm_calloc(npriorities, sizeof(struct evcallback_list));
	if (base->activequeues == NULL) {
		event_log("%s calloc failed", __FUNCTION__);
		return -1;
	}
	base->nactivequeues = npriorities;

	for (i = 0; i < base->nactivequeues; i++) {
		TAILQ_INIT(base->activequeues[i]);
	}

	return 0;
}

int evthread_make_base_notifiable(struct event_base* base)
{
	int r;
	if (!base) return -1;
	EVBASE_ACQUIRE_LOCK(base, th_base_lock);
	r = evthread_make_base_notifiable_nolock_(base);
	EVBASE_RELEASE_LOCK(base, th_base_lock);
	return r;
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

	base->active_later_queue.tqh_first = NULL;
	base->active_later_queue.tqh_last = &base->active_later_queue.tqh_first;

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

	for (i = 0; eventops[i] != NULL && base->evbase == NULL; i++) {
		if (cfg != NULL) {
			if (event_config_is_avoided_method(cfg, eventops[i]->name)) {
				continue;
			}

			if ((eventops[i]->features & cfg->require_features) != cfg->require_features) {
				continue;
			}
		}

		base->evsel = eventops[i];
		base->evbase = base->evsel->init(base);
	}

	if (base->evbase == NULL) {
		base->evsel = NULL;
		event_base_free(base);
		return NULL;
	}

	if (event_base_priority_init(base, 1) < 0) {
		event_base_free(base);
		return NULL;
	}

	if (evthread_lock_fns_.lock != NULL && (!cfg || (!cfg->flags & EVENT_BASE_FLAG_NOLOCK))) {
		 EVTHREAD_ALLOC_LOCK(base->th_base_lock, 0);
		 EVTHREAD_ALLOC_COND(base->current_event_cond);

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

int event_callback_active_nolock_(struct event_base* base, struct event_callback* evcb)
{
	int r = 1;

	if (evcb->evcb_flags & EVLIST_FINALIZING) return 0;

	switch (evcb->evcb_flags & (EVLIST_ACTIVE | EVLIST_ACTIVE_LATER)) {
	default:
		EVUTIL_ASSERT(0);
	case EVLIST_ACTIVE_LATER:
		event_queue_remove_active_later(base, evcb);
		r = 0;
		break;
	case EVLIST_ACTIVE:
		return 0;
	case 0:
		break;
	}

	event_queue_insert_active(base, evcb);

	if (EVBASE_NEED_NOTIFY(base)) {
		evthread_notify_base(base);
	}

	return r;
}

void event_active_nolock_(struct event *ev, int res, short ncalls)
{
	struct event_base* base;

	base = ev->ev_base;

	if (ev->ev_evcallback.evcb_flags == EVLIST_FINALIZING) return;

	switch ( ev->ev_evcallback.evcb_flags & (EVLIST_ACTIVE | EVLIST_ACTIVE_LATER))
	{
	default:
	case EVLIST_ACTIVE | EVLIST_ACTIVE_LATER:
		EVUTIL_ASSERT(0);
		break;
	case EVLIST_ACTIVE:
		ev->ev_res |= res;
		return;
	case EVLIST_ACTIVE_LATER:
		ev->ev_res |= res;
		break;
	case 0:
		ev->ev_res = res;
		break;
	}

	if (ev->ev_evcallback.evcb_pri < base->event_running_priority) {
		base->event_continue = 1;
	}

	if (ev->ev_events & EV_SIGNAL) {
		if (base->current_event == event_to_event_callback(ev) &&
			!EVBASE_IN_THREAD(base)) {
			++base->current_event_waiters;
			EVTHREAD_COND_WAIT(base->current_event_cond, base->th_base_lock);
		}
		ev->ev_.ev_signal.ev_ncalls = ncalls;
		ev->ev_.ev_signal.ev_pncalls = NULL;
	}

	event_callback_active_nolock_(base, event_to_event_callback(ev));
}





































