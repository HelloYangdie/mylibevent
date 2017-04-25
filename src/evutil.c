/*
 * evutil.c
 *
 *  Created on: Apr 24, 2017
 *      Author: magic
 */

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>

#include "event2/util.h"
#include "util_internal.h"
#include "log_internal.h"
#include "mm_internal.h"

static int evutil_issetugid()
{
	if (getuid() != geteuid()) {
		return 1;
	}

	if (getgid() != getegid()) {
		return 1;
	}

	return 0;
}

const char* evutil_getenv_(const char* varname)
{
	if (evutil_issetugid()) {
		return NULL;
	}

	return getenv(varname);
}

int evutil_make_socket_closeonexec(evutil_socket_t fd)
{
	int flags;
	if ((flags = fcntl(fd, F_GETFD, NULL)) < 0) {
		event_log("fcntl(%d, F_GETFD", fd);
		return -1;
	}

	if (!(flags & FD_CLOEXEC)) {
		if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
			event_log("fcntl(%d, F_SETFD", fd);
				return -1;
		}
	}

	return 0;
}

static int evutil_fast_socket_closeonexec(evutil_socket_t fd)
{
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
		event_log("fcntl(%d, F_SETFD)", fd);
		return -1;
	}

	return 0;
}

int evutil_make_socket_nonblocking(evutil_socket_t fd)
{
	int flags;
	if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
		event_log("fcntl(%d, F_GETFL)", fd);
		return -1;
	}

	if (!(flags & O_NONBLOCK)) {
		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
			event_log("fcntl(%d, F_SETFL)", fd);
			return -1;
		}
	}

	return 0;
}

static int evutil_fast_socket_nonblocking(evutil_socket_t fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		event_warn("fcntl(%d, F_SETFL)", fd);
		return -1;
	}
	return 0;
}

int evutil_make_internal_pipe_(evutil_socket_t fd[2])
{
	if (pipe(fd) == 0) {
		if (evutil_fast_socket_nonblocking(fd[0]) < 0 ||
			evutil_fast_socket_nonblocking(fd[1]) < 0 ||
			evutil_fast_socket_closeonexec(fd[0]) < 0 ||
			evutil_fast_socket_closeonexec(fd[1]) < 0) {
			close(fd[0]);
			close(fd[1]);
			fd[0] = fd[1] = -1;
			return -1;
		}
		return 0;
	} else {
		event_log("%s: pipe", __FUNCTION__);
	}

	fd[0] = fd[1] = -1;
	return -1;
}





























