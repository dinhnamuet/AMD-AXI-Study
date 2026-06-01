#include "itimer.h"

static struct {
	uint32_t        (*get_tick)(void *ctx);
	void             *ctx;
	struct list_head  active_list;
} mgr;

static void insert_sorted(struct itimer_node *n)
{
	struct itimer_node *pos;

	list_for_each_entry(pos, &mgr.active_list, node) {
		if ((int32_t)(n->expiry - pos->expiry) < 0) {
			list_add_tail(&n->node, &pos->node);
			return;
		}
	}
	list_add_tail(&n->node, &mgr.active_list);
}

static bool itimer_node_linked(struct itimer_node *n)
{
	return n->node.next && n->node.next != &n->node;
}

void itimer_init(uint32_t (*get_tick)(void *ctx), void *ctx)
{
	mgr.get_tick = get_tick;
	mgr.ctx      = ctx;
	INIT_LIST_HEAD(&mgr.active_list);
}

void itimer_node_init(struct itimer_node *n, uint32_t period, bool repeat,
		      itimer_fn_t fn)
{
	INIT_LIST_HEAD(&n->node);
	n->period = period;
	n->repeat = repeat;
	n->fn     = fn;
}

void itimer_process(void)
{
	uint32_t now = mgr.get_tick(mgr.ctx);
	struct itimer_node *n;

	while (!list_empty(&mgr.active_list)) {
		n = list_first_entry(&mgr.active_list, struct itimer_node, node);

		if ((int32_t)(now - n->expiry) < 0)
			break;

		list_del_init(&n->node);

		if (n->fn)
			n->fn(n);

		if (n->repeat) {
			n->expiry += n->period;
			insert_sorted(n);
		}
	}
}

int itimer_start(struct itimer_node *n)
{
	if (!n)
		return -EINVAL;
	if (n->repeat && !n->period)
		return -EINVAL;
	if (itimer_node_linked(n))
		return EOK;

	n->expiry = mgr.get_tick(mgr.ctx) + n->period;
	insert_sorted(n);
	return EOK;
}

int itimer_stop(struct itimer_node *n)
{
	if (!n || !itimer_node_linked(n))
		return EOK;

	list_del_init(&n->node);
	return EOK;
}

void itimer_set_timeout(struct itimer_node *n, uint32_t ticks, itimer_fn_t fn)
{
	itimer_node_init(n, ticks, false, fn);
	itimer_start(n);
}
