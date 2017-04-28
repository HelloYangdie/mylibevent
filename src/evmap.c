/*
 * evmap.c
 *
 *  Created on: Apr 25, 2017
 *      Author: magic
 */

#include "evmap_internal.h"
#include "changelist_internal.h"
#include "event_internal.h"

struct event_changelist_fdinfo {
	int idxplus1;
};

struct evmap_io {
	struct event_dlist events;
	uint16_t nread;
	uint16_t nwrite;
	uint16_t nclose;
};

struct evmap_signal {
	struct event_dlist events;
};

void evmap_signal_initmap_(struct event_signal_map* ctx)
{
	ctx->nentries = 0;
	ctx->entries = NULL;
}

void evmap_io_initmap_(struct event_io_map* ctx)
{
	evmap_signal_initmap_(ctx);
}

static struct event_changelist_fdinfo*
event_change_get_fdinfo(struct event_base* base, const struct event_change* change)
{
	char* ptr;
	if (change->read_change & EV_CHANGE_SIGNAL) {
		struct evmap_signal* ctx;
		ctx = (struct evmap_signal*)(base->sigmap.entries[change->fd]);
		ptr = (char*)ctx + sizeof(struct evmap_signal);
	} else {
		struct evmap_io* ctx;
		ctx = (struct evmap_io*)(base->io.entries[change->fd]);
		ptr = (char*)ctx + sizeof(struct evmap_io);
	}

	return (void*)ptr;
}

void event_changelist_init_(struct event_changelist* changelist)
{
	changelist->changes = NULL;
	changelist->changes_size = 0;
	changelist->n_changes = 0;
}

void event_changelist_remove_all_(struct event_changelist* changelist, struct event_base* base)
{
	int i;
	for (i = 0; i < changelist->n_changes; i++) {
		struct event_change* ch = &(changelist->changes[i]);
		struct event_changelist_fdinfo* fdinfo = event_change_get_fdinfo(base, ch);
		EVUTIL_ASSERT(fdinfo->idxplus1 == (i+1));
		fdinfo->idxplus1 = 0;
	}

	changelist->n_changes = 0;
}

void evmap_io_active_(struct event_base* base, evutil_socket_t fd, short events)
{
	struct event_io_map* io = &base->io;
	struct evmap_io* ctx;
	struct event* ev;

	if (fd < 0 || fd >= io->nentries) return;

	ctx = (struct evmap_io*)(io->entries[fd]);
	if (ctx == NULL) return;

	for (ev = ctx->events.lh_first; ev != NULL; ev = ev->ev_.ev_io.ev_io_next.le_next) {
		if (ev->ev_events & events) {
			event_active_nolock_(ev, ev->ev_events & events, 1);
		}
	}
}





































