/*
 * evmap.c
 *
 *  Created on: Apr 25, 2017
 *      Author: magic
 */

#include "evmap_internal.h"

void evmap_signal_initmap_(struct event_signal_map* ctx)
{
	ctx->nentries = 0;
	ctx->entries = NULL;
}

void evmap_io_initmap_(struct event_io_map* ctx)
{
	evmap_signal_initmap_(ctx);
}

void event_changelist_init_(struct event_changelist* changelist)
{
	changelist->changes = NULL;
	changelist->changes_size = 0;
	changelist->n_changes = 0;
}
