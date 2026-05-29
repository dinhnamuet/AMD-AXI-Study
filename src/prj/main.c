#include "xparameters.h"
#include "intc.h"
#include "uart.h"

int main(void)
{
	const u8 *rx;
	struct uartlite uart;

	intc_init_once(XPAR_MICROBLAZE_0_AXI_INTC_BASEADDR);
	uart_init(&uart, XPAR_AXI_UARTLITE_0_DEVICE_ID, XPAR_INTC_0_UARTLITE_0_VEC_ID);

	while (1) {
		rx = (u8 *)msgq_peek(&uart_rxq);
		if (rx) {
			uart_poll_out(&uart, rx, 10);
			msgq_skip(&uart_rxq);
		}
	}
	return 0;
}
