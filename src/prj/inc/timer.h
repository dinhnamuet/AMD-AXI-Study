#pragma once

#include "define.h"
#include "gcc.h"
#include <stdbool.h>
#include "xil_types.h"

struct timer;

struct timer_alarm {
	void (*callback)(struct timer *timer, u32 now, void *user_data);
	void *user_data;
};

struct timer {
	u32 base;
	u32 freq;
	bool has_alarm;
	struct timer_alarm alarm;
};

int  timer_init(struct timer *timer, u32 base, u32 freq, int irq, bool has_alarm);
u32  timer_get_tick(struct timer *timer);
void timer_start(struct timer *timer);
void timer_stop(struct timer *timer);
int  timer_set_alarm(struct timer *timer, u32 alarm_ticks,
		     void (*cb)(struct timer *, u32, void *), void *user_data);
void timer_cancel_alarm(struct timer *timer);

static ALWAYS_INLINE u32 timer_get_freq(struct timer *timer)
{
	return timer->freq;
}

static ALWAYS_INLINE u32 us_to_ticks(struct timer *timer, u64 us)
{
	u64 ticks = (us * timer->freq) / USEC_PER_SEC;

	return (ticks > UINT32_MAX) ? UINT32_MAX : (u32)ticks;
}

static ALWAYS_INLINE u64 ticks_to_us(struct timer *timer, u32 ticks)
{
	return ((u64)ticks * USEC_PER_SEC) / timer->freq;
}
