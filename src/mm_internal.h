/*
 * mm_internal.h
 *
 *  Created on: Apr 21, 2017
 *      Author: magic
 */

#ifndef SRC_MM_INTERNAL_H_
#define SRC_MM_INTERNAL_H_

#include <sys/types.h>

#ifndef EVENT__DISABLE_MM_REPLACEMENT
/* Internal use only: Memory allocation functions. We give them nice short
 * mm_names for our own use, but make sure that the symbols have longer names
 * so they don't conflict with other libraries (like, say, libmm). */

/** Allocate uninitialized memory.
 *
 * @return On success, return a pointer to sz newly allocated bytes.
 *     On failure, set errno to ENOMEM and return NULL.
 *     If the argument sz is 0, simply return NULL.
 */
void *event_mm_malloc_(size_t sz);

/** Allocate memory initialized to zero.
 *
 * @return On success, return a pointer to (count * size) newly allocated
 *     bytes, initialized to zero.
 *     On failure, or if the product would result in an integer overflow,
 *     set errno to ENOMEM and return NULL.
 *     If either arguments are 0, simply return NULL.
 */
void *event_mm_calloc_(size_t count, size_t size);

/** Duplicate a string.
 *
 * @return On success, return a pointer to a newly allocated duplicate
 *     of a string.
 *     Set errno to ENOMEM and return NULL if a memory allocation error
 *     occurs (or would occur) in the process.
 *     If the argument str is NULL, set errno to EINVAL and return NULL.
 */
char *event_mm_strdup_(const char *str);

void *event_mm_realloc_(void *p, size_t sz);
void event_mm_free_(void *p);
#define mm_malloc(sz) event_mm_malloc_(sz)
#define mm_calloc(count, size) event_mm_calloc_((count), (size))
#define mm_strdup(s) event_mm_strdup_(s)
#define mm_realloc(p, sz) event_mm_realloc_((p), (sz))
#define mm_free(p) event_mm_free_(p)
#else
#define mm_malloc(sz) malloc(sz)
#define mm_calloc(n, sz) calloc((n), (sz))
#define mm_strdup(s) strdup(s)
#define mm_realloc(p, sz) realloc((p), (sz))
#define mm_free(p) free(p)
#endif


#endif /* SRC_MM_INTERNAL_H_ */
