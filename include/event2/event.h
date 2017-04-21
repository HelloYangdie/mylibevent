/*
 * event.h
 *
 *  Created on: Apr 20, 2017
 *      Author: magic
 */

#ifndef INCLUDE_EVENT2_EVENT_H_
#define INCLUDE_EVENT2_EVENT_H_

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

#endif /* INCLUDE_EVENT2_EVENT_H_ */
