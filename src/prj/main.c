#include "xparameters.h"
#include "intc.h"
#include "uart.h"
#include "timer.h"
#include "xil_printf.h"

static void timer_cb(struct timer *timer, void *args)
{
	struct uartlite *serdev = args;

	uart_poll_out(serdev, "hehe\n", 5);
}

static void alarm_cb(struct timer *timer, u32 now, void *args)
{
	struct uartlite *serdev = args;

	uart_poll_out(serdev, "hihi\n", 5);
	timer_set_alarm(timer, us_to_ticks(timer, 5000000), alarm_cb, serdev);
}

int main(void)
{
	const u8 *rx;
	struct uartlite uart;
	struct timer timer;

	intc_init_once(XPAR_MICROBLAZE_0_AXI_INTC_BASEADDR);
	uart_init(&uart, XPAR_AXI_UARTLITE_0_DEVICE_ID, XPAR_INTC_0_UARTLITE_0_VEC_ID);
	timer_init(&timer, XPAR_AXI_TIMER_0_BASEADDR, XPAR_TMRCTR_0_CLOCK_FREQ_HZ,
			XPAR_INTC_0_TMRCTR_0_VEC_ID, true);
	timer_set_period(&timer, 1000000);
	timer.top_callback = timer_cb;
	timer.top_user_data = &uart;
	timer_set_alarm(&timer, us_to_ticks(&timer, 5000000), alarm_cb, &uart);
	timer_start(&timer);

	while (1) {
		rx = (u8 *)msgq_peek(&uart_rxq);
		if (rx) {
			uart_poll_out(&uart, rx, 10);
			msgq_skip(&uart_rxq);
		}
	}
	return 0;
}
