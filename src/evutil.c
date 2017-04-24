/*
 * evutil.c
 *
 *  Created on: Apr 24, 2017
 *      Author: magic
 */

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

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
































