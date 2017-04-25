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

#define MAX_EPOLL_TIMEOUT_MSEC (35*60*1000)
#define INITIAL_NEVENT 32
#define MAX_NEVENT 4096

static void *epoll_init(struct event_base *);
static int epoll_nochangelist_add(struct event_base *base, evutil_socket_t fd,
    short old, short events, void *p);
static int epoll_nochangelist_del(struct event_base *base, evutil_socket_t fd,
    short old, short events, void *p);

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
}








































