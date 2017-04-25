/*
 * changelist_internal.h
 *
 *  Created on: Apr 25, 2017
 *      Author: magic
 */

#ifndef SRC_CHANGELIST_INTERNAL_H_
#define SRC_CHANGELIST_INTERNAL_H_

#include "include/event2/util.h"

struct event_change {
	evutil_socket_t fd;
	short old_events;
	uint8_t read_change;
	uint8_t write_change;
	uint8_t close_change;
};

/* If set, add the event. */
#define EV_CHANGE_ADD     0x01
/* If set, delete the event.  Exclusive with EV_CHANGE_ADD */
#define EV_CHANGE_DEL     0x02
/* If set, this event refers a signal, not an fd. */
#define EV_CHANGE_SIGNAL  EV_SIGNAL
/* Set for persistent events.  Currently not used. */
#define EV_CHANGE_PERSIST EV_PERSIST
/* Set for adding edge-triggered events. */
#define EV_CHANGE_ET      EV_ET

#endif /* SRC_CHANGELIST_INTERNAL_H_ */
