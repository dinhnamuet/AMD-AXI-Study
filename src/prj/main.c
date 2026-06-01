#include "xparameters.h"
#include "intc.h"
#include "uart.h"
#include "timer.h"
#include "itimer.h"
#include "xil_printf.h"

static uint32_t hw_get_tick(void *ctx)
{
	return timer_get_tick((struct timer *)ctx);
}

int main(void)
{
	const u8 *rx;
	struct uartlite uart;
	struct timer timer;

	intc_init_once(XPAR_MICROBLAZE_0_AXI_INTC_BASEADDR);
	timer_init(&timer, XPAR_AXI_TIMER_0_BASEADDR, XPAR_TMRCTR_0_CLOCK_FREQ_HZ,
			XPAR_INTC_0_TMRCTR_0_VEC_ID, false);
	timer_start(&timer);
	itimer_init(hw_get_tick, &timer);
	uart_init(&uart, XPAR_AXI_UARTLITE_0_DEVICE_ID, XPAR_INTC_0_UARTLITE_0_VEC_ID,
			us_to_ticks(&timer, 5000));

	while (1) {
		itimer_process();
		rx = (u8 *)msgq_peek(&uart_rxq);
		if (rx) {
			uart_poll_out(&uart, rx, 10);
			msgq_skip(&uart_rxq);
		}
	}
	return 0;
}
