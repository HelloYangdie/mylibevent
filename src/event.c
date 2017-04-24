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

static int gettime(struct event_base* base, struct timeval* tp)
{
	EVENT_BASE_ASSERT_LOCKED(base);

	if (base->tv_cache.tv_sec) {
		*tp = base->tv_cache;
		return 0;
	}


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







































