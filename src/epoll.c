/*
 * epoll.c
 *
 *  Created on: Apr 25, 2017
 *      Author: magic
 */

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <errno.h>

#include "event_internal.h"
#include "log_internal.h"
#include "evsignal_internal.h"
#include "changelist_internal.h"
#include "epolltable_internal.h"

#define MAX_EPOLL_TIMEOUT_MSEC (35*60*1000)
#define INITIAL_NEVENT 32
#define MAX_NEVENT 4096

static void *epoll_init(struct event_base *);
static int epoll_nochangelist_add(struct event_base *base, evutil_socket_t fd,
    short old, short events, void *p);
static int epoll_nochangelist_del(struct event_base *base, evutil_socket_t fd,
    short old, short events, void *p);
static int epoll_dispatch(struct event_base *, struct timeval *);
static void epoll_dealloc(struct event_base *);

struct epollop {
	struct epoll_event* events;
	int nevents;
	int epfd;
	int timerfd;
};

static const struct eventop epollops_changelist = {
	"epoll (with changelist)",
};

const struct eventop epollops = {
	"epoll",
	epoll_init,
	epoll_nochangelist_add,
	epoll_nochangelist_del,
};

static void* epoll_init(struct event_base* base)
{
	int epfd = -1;
	struct epollop* epoll_op;

	if ((epfd = epoll_create(1)) == -1) {
		if (errno != ENOSYS) {
			event_log("epoll_create");
		}
		return NULL;
	}

	evutil_make_socket_closeonexec(epfd);

	if (!(epoll_op = mm_calloc(1, sizeof(struct epollop)))) {
		close(epfd);
		return NULL;
	}

	epoll_op->epfd =epfd;
	epoll_op->events = mm_calloc(INITIAL_NEVENT, sizeof(struct epoll_event));
	if (epoll_op->events == NULL) {
		mm_free(epoll_op);
		close(epfd);
		return NULL;
	}
	epoll_op->nevents = INITIAL_NEVENT;

	if ( (base->flags & EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST) != 0 ||
		 ((base->flags & EVENT_BASE_FLAG_IGNORE_ENV) == 0 &&
		 evutil_getenv_("EVENT_EPOLL_USE_CHANGELIST") != NULL)) {
		base->evsel = &epollops_changelist;
	}

	if ( (base->flags & EVENT_BASE_FLAG_PRECISE_TIMER) &&
		 base->monotonic_timer.monotonic_clock == CLOCK_MONOTONIC) {
		int fd;
		fd = epoll_op->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC);
		if (epoll_op->timerfd >= 0) {
			struct epoll_event epev;
			memset(&epev, 0x00, sizeof(epev));
			epev.data.fd = fd;
			epev.events = EPOLLIN;
			if (epoll_ctl(epoll_op->epfd, EPOLL_CTL_ADD, fd, &epev) < 0) {
				event_log("epoll_ctl(timerfd");
				close(fd);
				epoll_op->timerfd = -1;
			}
		} else {
			if (errno != EINVAL && errno != ENOSYS) {
				event_log("timerfd_create");
			}
			epoll_op->timerfd = -1;
		}
	} else {
		epoll_op->timerfd = -1;
	}

	evsig_init_(base);

	return epoll_op;
}

static int epoll_apply_one_change(struct event_base* base, struct epollop* epollop, const struct event_change* ch)
{
	struct epoll_event epev;
	int op, events = 0;
	int idx;

	idx = EPOLL_OP_TABLE_INDEX(ch);
	op = epoll_op_table[idx].op;
	events = epoll_op_table[idx].events;

	if (!events) {
		EVUTIL_ASSERT(op == 0);
		return 0;
	}

	if ((ch->read_change | ch->write_change) & EV_CHANGE_ET) {
		events |= EPOLLET;
	}

	epev.data.fd = ch->fd;
	epev.events = events;
	if (epoll_ctl(epollop->epfd, op, ch->fd, &epev) == 0) {
		event_log("epoll_ctl ok");
		return 0;
	}

	switch (op) {
	case EPOLL_CTL_MOD:
		if (errno == ENOENT) {
			if (epoll_ctl(epollop->epfd, EPOLL_CTL_ADD, ch->fd, &epev) == -1) {
				event_log("Epoll MOD(%d) on %d retried as ADD; that failed too",
					(int)epev.events, ch->fd);
				return -1;
			} else {
				event_log(("Epoll MOD(%d) on %d retried as ADD; succeeded.",
					(int)epev.events,
					ch->fd));
				return 0;
			}
		}
		break;
	case EPOLL_CTL_ADD:
		if (errno == EEXIST) {
			if (epoll_ctl(epollop->epfd, EPOLL_CTL_MOD, ch->fd, &epev) == -1) {
				event_log("Epoll ADD(%d) on %d retried as MOD; that failed too",
					(int)epev.events, ch->fd);
				return -1;
			} else {
				event_log(("Epoll ADD(%d) on %d retried as MOD; succeeded.",
					(int)epev.events,
					ch->fd));
				return 0;
			}
		}
		break;
	case EPOLL_CTL_DEL:
		if (errno == ENOENT || errno == EBADF || errno == EPERM) {
			event_log(("Epoll DEL(%d) on fd %d gave %s: DEL was unnecessary.",
				(int)epev.events,
				ch->fd,
				strerror(errno)));
			return 0;
		}
		break;
	default:
		break;
	}
}

static int epoll_apply_changes(struct event_base* base)
{
	struct event_changelist* changelist = &base->changelist;
	struct epollop* epoll_op = base->evbase;
	struct event_change* ch;
	int r = 0;
	int i = 0;

	for (i = 0; i < changelist->n_changes; i++) {
		ch = &(changelist->changes[i]);
		if (epoll_apply_one_change(base, epoll_op, ch) < 0)
			r = -1;
	}

	return r;
}

static int epoll_nochangelist_add(struct event_base* base, evutil_socket_t fd, short old, short events, void* p)
{
	struct event_change ch;
	ch.fd = fd;
	ch.old_events = old;
	ch.read_change = ch.write_change = ch.close_change = 0;
	if (events & EV_WRITE) {
		ch.write_change = EV_CHANGE_ADD | (events & EV_ET);
	}
	if (events & EV_READ) {
		ch.read_change = EV_CHANGE_ADD | (events & EV_ET);
	}
	if (events & EV_CLOSED) {
		ch.close_change = EV_CHANGE_ADD | (events & EV_ET);
	}

	return epoll_apply_one_change(base, base->evbase, &ch);
}

static int epoll_nochangelist_del(struct event_base* base, evutil_socket_t fd, short old, short events, void* p)
{
	struct event_change ch;
	ch.fd = fd;
	ch.old_events = old;
	ch.read_change = ch.write_change = ch.close_change = 0;
	if (events & EV_WRITE)
		ch.write_change = EV_CHANGE_DEL;
	if (events & EV_READ)
		ch.read_change = EV_CHANGE_DEL;
	if (events & EV_CLOSED)
		ch.close_change = EV_CHANGE_DEL;

	return epoll_apply_one_change(base, base->evbase, &ch);
}

static int epoll_dispatch(struct event_base *base, struct timeval *tv)
{
	struct epollop* epoll_op = base->evbase;
	struct epoll_event* events = epoll_op->events;
	int i, res;
	long timeout = -1;

	if (epoll_op->timerfd >= 0) {
		struct itimerspec is;
		is.it_interval.tv_sec = 0;
		is.it_interval.tv_nsec = 0;
		if (tv == NULL) {
			is.it_value.tv_nsec = 0;
			is.it_value.tv_sec = 0;
		} else {
			if (tv->tv_sec == 0 && tv->tv_usec == 0) {
				timeout = 0;
			}
			is.it_value.tv_sec = tv->tv_sec;
			is.it_value.tv_nsec = tv->tv_usec * 1000;
		}

		if (timerfd_settime(epoll_op->timerfd, 0, &is, NULL) < 0) {
			event_log("timerfd_settime");
		}
	} else {
		if (tv != NULL) {
			timeout = evutil_tv_to_msec_(tv);
			if (timeout < 0 || timeout > MAX_EPOLL_TIMEOUT_MSEC) {
				timeout = MAX_EPOLL_TIMEOUT_MSEC;
			}
		}
	}

	epoll_apply_changes(base);
	event_changelist_remove_all_(&base->changelist, base);

	res = epoll_wait(epoll_op->epfd, events, epoll_op->nevents, timeout);

	if (res == -1) {
		if (errno != EINTR) {
			event_log("epoll_wait");
			return -1;
		}
		return 0;
	}

	EVUTIL_ASSERT(res <= epoll_op->nevents);

	for (i = 0; i < res; i++) {
		int what = events[i].events;
		short ev = 0;
		if (events[i].data.fd == epoll_op->timerfd) {
			continue;
		}

		if (what & (EPOLLHUP|EPOLLERR)) {
			ev = EV_READ | EV_WRITE;
		} else {
			if (what & EPOLLIN)
				ev |= EV_READ;
			if (what & EPOLLOUT)
				ev |= EV_WRITE;
			if (what & EPOLLRDHUP)
				ev |= EV_CLOSED;
		}

		if (!ev) continue;

		evmap_io_active_(base, events[i].data.fd, ev | EV_ET);
	}
}





































