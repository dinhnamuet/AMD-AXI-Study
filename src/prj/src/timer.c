#include "timer.h"
#include "intc.h"
#include "io.h"

/* AXI Timer v2.0 register offsets (Xilinx PG079) */
#define TCSR0_OFFSET  0x00U
#define TLR0_OFFSET   0x04U
#define TCR0_OFFSET   0x08U
#define TCSR1_OFFSET  0x10U
#define TLR1_OFFSET   0x14U
#define TCR1_OFFSET   0x18U

/* TCSRx bit definitions */
#define TCSR_MDT    BIT(0)
#define TCSR_UDT    BIT(1)
#define TCSR_GENT   BIT(2)
#define TCSR_CAPT   BIT(3)
#define TCSR_ARHT   BIT(4)
#define TCSR_LOAD   BIT(5)
#define TCSR_ENIT   BIT(6)
#define TCSR_ENT    BIT(7)
#define TCSR_TINT   BIT(8)
#define TCSR_PWMA   BIT(9)
#define TCSR_ENALL  BIT(10)
#define TCSR_CASC   BIT(11)

/* Timer0: auto-reload, count-down, generate, interrupt enabled */
#define TCSR0_DEFAULT  (TCSR_ENIT | TCSR_ARHT | TCSR_GENT | TCSR_UDT)
/* Timer1: count-down, generate, interrupt enabled (one-shot alarm) */
#define TCSR1_DEFAULT  (TCSR_ENIT | TCSR_GENT | TCSR_UDT)

static inline u32 timer_read(struct timer *timer, u32 offset)
{
	return io_read32(timer->base + offset);
}

static inline void timer_write(struct timer *timer, u32 val, u32 offset)
{
	io_write32(timer->base + offset, val);
}

static void timer_isr(void *args)
{
	struct timer *timer = (struct timer *)args;
	u32 tcsr;

	if (timer->has_alarm) {
		tcsr = timer_read(timer, TCSR1_OFFSET);
		if (tcsr & TCSR_TINT) {
			/* W1C: clear TINT and reset control bits to default */
			timer_write(timer, TCSR1_DEFAULT | TCSR_TINT, TCSR1_OFFSET);

			if (timer->alarm.callback) {
				u32 now = timer_read(timer, TCR0_OFFSET);
				void (*cb)(struct timer *, u32, void *) = timer->alarm.callback;
				timer->alarm.callback = NULL;
				cb(timer, now, timer->alarm.user_data);
			}
		}
	}

	tcsr = timer_read(timer, TCSR0_OFFSET);
	if (tcsr & TCSR_TINT) {
		/* W1C: writing back tcsr with TINT=1 clears the interrupt */
		timer_write(timer, tcsr, TCSR0_OFFSET);
		atomic_fetch_add_explicit(&timer->ticks, 1, memory_order_relaxed);

		if (timer->top_callback)
			timer->top_callback(timer, timer->top_user_data);
	}
}

int timer_init(struct timer *timer, u32 base, u32 freq, int irq, bool has_alarm)
{
	if (!timer || !freq)
		return -EINVAL;

	timer->base          = base;
	timer->freq          = freq;
	timer->has_alarm     = has_alarm;
	timer->top_callback  = NULL;
	timer->top_user_data = NULL;
	timer->alarm.callback  = NULL;
	timer->alarm.user_data = NULL;
	atomic_init(&timer->ticks, 0);

	/* Load max top value and reset timer0, leave stopped */
	timer_write(timer, UINT32_MAX, TLR0_OFFSET);
	timer_write(timer, TCSR0_DEFAULT | TCSR_LOAD, TCSR0_OFFSET);
	timer_write(timer, TCSR0_DEFAULT, TCSR0_OFFSET);

	if (has_alarm)
		timer_write(timer, TCSR1_DEFAULT, TCSR1_OFFSET);

	return request_irq(irq, timer_isr, timer);
}

void timer_start(struct timer *timer)
{
	u32 tcsr = TCSR0_DEFAULT | TCSR_ENT;

	/* Start both timers synchronously if an alarm is armed */
	if (timer->has_alarm && timer->alarm.callback)
		tcsr |= TCSR_ENALL;

	timer_write(timer, tcsr, TCSR0_OFFSET);
}

void timer_stop(struct timer *timer)
{
	if (timer->has_alarm)
		timer_write(timer, TCSR1_DEFAULT, TCSR1_OFFSET);
	timer_write(timer, TCSR0_DEFAULT, TCSR0_OFFSET);
}

int timer_set_period(struct timer *timer, u64 period_us)
{
	u32 ticks = us_to_ticks(timer, period_us);
	u32 tcsr;

	if (!ticks)
		return -EINVAL;

	tcsr = timer_read(timer, TCSR0_OFFSET);
	timer_write(timer, ticks, TLR0_OFFSET);
	timer_write(timer, tcsr | TCSR_LOAD, TCSR0_OFFSET);
	timer_write(timer, tcsr, TCSR0_OFFSET);
	return 0;
}

int timer_set_alarm(struct timer *timer, u32 alarm_ticks,
		    void (*cb)(struct timer *, u32, void *), void *user_data)
{
	u32 tcsr;

	if (!timer->has_alarm || !cb)
		return -EINVAL;
	if (timer->alarm.callback)
		return -EBUSY;
	if (!alarm_ticks)
		return -EINVAL;

	timer->alarm.callback  = cb;
	timer->alarm.user_data = user_data;

	timer_write(timer, alarm_ticks, TLR1_OFFSET);
	timer_write(timer, TCSR1_DEFAULT | TCSR_LOAD, TCSR1_OFFSET);

	/* Enable alarm timer only if main timer is already running */
	tcsr = timer_read(timer, TCSR0_OFFSET);
	timer_write(timer, TCSR1_DEFAULT | (tcsr & TCSR_ENT), TCSR1_OFFSET);
	return 0;
}

void timer_cancel_alarm(struct timer *timer)
{
	timer_write(timer, TCSR1_DEFAULT, TCSR1_OFFSET);
	timer->alarm.callback  = NULL;
	timer->alarm.user_data = NULL;
}

unsigned long timer_get_ticks(struct timer *timer)
{
	return atomic_load_explicit(&timer->ticks, memory_order_relaxed);
}
