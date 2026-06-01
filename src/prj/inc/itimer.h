#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "list.h"
#include "define.h"

struct itimer_node;
typedef void (*itimer_fn_t)(struct itimer_node *n);

struct itimer_node {
	struct list_head  node;
	uint32_t          expiry;
	uint32_t          period;
	itimer_fn_t       fn;
	bool              repeat;
};

void itimer_init(uint32_t (*get_tick)(void *ctx), void *ctx);

void itimer_node_init(struct itimer_node *n, uint32_t period, bool repeat,
		      itimer_fn_t fn);

void itimer_process(void);

int  itimer_start(struct itimer_node *n);
int  itimer_stop(struct itimer_node *n);

/* Convenience: init + start a one-shot timer in one call. */
void itimer_set_timeout(struct itimer_node *n, uint32_t ticks, itimer_fn_t fn);
