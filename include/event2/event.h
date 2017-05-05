/*
 * event.h
 *
 *  Created on: Apr 20, 2017
 *      Author: magic
 */

#ifndef INCLUDE_EVENT2_EVENT_H_
#define INCLUDE_EVENT2_EVENT_H_

/**
 * @name event flags
 *
 * Flags to pass to event_new(), event_assign(), event_pending(), and
 * anything else with an argument of the form "short events"
 */
/**@{*/
/** Indicates that a timeout has occurred.  It's not necessary to pass
 * this flag to event_for new()/event_assign() to get a timeout. */
#define EV_TIMEOUT	0x01
/** Wait for a socket or FD to become readable */
#define EV_READ		0x02
/** Wait for a socket or FD to become writeable */
#define EV_WRITE	0x04
/** Wait for a POSIX signal to be raised*/
#define EV_SIGNAL	0x08
/**
 * Persistent event: won't get removed automatically when activated.
 *
 * When a persistent event with a timeout becomes activated, its timeout
 * is reset to 0.
 */
#define EV_PERSIST	0x10
/** Select edge-triggered behavior, if supported by the backend. */
#define EV_ET		0x20
/**
 * If this option is provided, then event_del() will not block in one thread
 * while waiting for the event callback to complete in another thread.
 *
 * To use this option safely, you may need to use event_finalize() or
 * event_free_finalize() in order to safely tear down an event in a
 * multithreaded application.  See those functions for more information.
 *
 * THIS IS AN EXPERIMENTAL API. IT MIGHT CHANGE BEFORE THE LIBEVENT 2.1 SERIES
 * BECOMES STABLE.
 **/
#define EV_FINALIZE     0x40
/**
 * Detects connection close events.  You can use this to detect when a
 * connection has been closed, without having to read all the pending data
 * from a connection.
 *
 * Not all backends support EV_CLOSED.  To detect or require it, use the
 * feature flag EV_FEATURE_EARLY_CLOSE.
 **/
#define EV_CLOSED	0x80


#define EVENT_MAX_PRIORITIES 256

enum event_method_feature {
	/** Require an event method that allows edge-triggered events with EV_ET. */
	EV_FEATURE_ET = 0X01,
	/** Require an event method where having one event triggered among
	 * many is [approximately] an O(1) operation. This excludes (for
	 * example) select and poll, which are approximately O(N) for N
	 * equal to the total number of possible events. */
	EV_FEATURE_O1 = 0X02,
	/** Require an event method that allows file descriptors as well as
	 * sockets. */
	EV_FEATURE_FDS = 0X04,
	/** Require an event method that allows you to use EV_CLOSED to detect
	 * connection close without the necessity of reading all the pending data.
	 *
	 * Methods that do support EV_CLOSED may not be able to provide support on
	 * all kernel versions.
	 **/
	EV_FEATURE_EARLY_CLOSE = 0X08
};

enum event_base_config_flag {

	EVENT_BASE_FLAG_NOLOCK = 0X01,
	/** Do not check the EVENT_* environment variables when configuring
		an event_base  */
	EVENT_BASE_FLAG_IGNORE_ENV = 0X02,

	EVENT_BASE_FLAG_STARTUP_IOCP = 0X04,

	EVENT_BASE_FLAG_NO_CACHE_TIME = 0X08,

	EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST = 0X10,

	EVENT_BASE_FLAG_PRECISE_TIMER = 0X20
};

struct event_config* event_config_new(void);

struct event_base* event_base_new_with_config(const struct event_config*);

void event_config_free(struct event_config* cfg);

typedef int (*event_base_foreach_event_cb)(const struct event_base*, const struct event*, void*);
typedef void (*event_callback_fn)(evutil_socket_t, short, void *);

int event_assign(struct event *, struct event_base *, evutil_socket_t, short, event_callback_fn, void *);

int	event_priority_set(struct event *, int);

void event_base_free(struct event_base *);

int event_base_priority_init(struct event_base* ,int);

#endif /* INCLUDE_EVENT2_EVENT_H_ */


































