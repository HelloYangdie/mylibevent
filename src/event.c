/*
 * event.c
 *
 *  Created on: Apr 20, 2017
 *      Author: magic
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mm_internal.h"
#include "event_internal.h"

static void *(*mm_malloc_fn_)(size_t sz) = NULL;
static void *(*mm_realloc_fn_)(void *p, size_t sz) = NULL;
static void (*mm_free_fn_)(void *p) = NULL;

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
