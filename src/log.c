/*
 * log.c
 *
 *  Created on: Apr 24, 2017
 *      Author: magic
 */

#include <stdio.h>
#include <string.h>

void event_log(const char* fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	memset(buf, 0x00, 1024);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	fprintf(stderr, "%s\n", buf);
}
