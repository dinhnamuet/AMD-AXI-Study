#include "uart.h"
#include "xparameters.h"
#include "xil_types.h"
#include "xstatus.h"
#include "define.h"
#include "io.h"
#include "intc.h"
#include "mb_interface.h"

MSGQ_DEFINE(uart_rxq, UART_BUF_SIZE_CAL, \
		UART_QUEUE_CAPACITY, UART_BUF_ALIGN);

#define UART_TX_FIFO_RST   BIT(0)
#define UART_RX_FIFO_RST   BIT(1)
#define UART_IRQ_ENABLE    BIT(4)

#define UART_RX_FIFO_OFFSET    0x00U
#define UART_TX_FIFO_OFFSET    0x04U
#define UART_STATUS_OFFSET     0x08U
#define UART_CTRL_OFFSET       0x0CU

#define UART_SR_RX_FIFO_VALID  BIT(0)
#define UART_SR_RX_FIFO_FULL   BIT(1)
#define UART_SR_TX_FIFO_EMPTY  BIT(2)
#define UART_SR_TX_FIFO_FULL   BIT(3)
#define UART_SR_IRQ_ENABLE     BIT(4)
#define UART_SR_OVERRUN_ERROR  BIT(5)
#define UART_SR_FRAME_ERROR    BIT(6)
#define UART_SR_PARITY_ERROR   BIT(7)

static void uart_idle_cb(struct itimer_node *n)
{
	struct uartlite *uart = container_of(n, struct uartlite, idle_timer);

	if (!uart->rx_pos)
		return;

	msgq_put_safe(&uart_rxq, uart->rx_buf);
	uart->rx_pos = 0;
	compiler_barrier();
}

static void uart_isr(void *arg)
{
	u32 reg_status;
	XUartLite *uartl = (XUartLite *)arg;
	struct uartlite *uart_ctx = container_of(uartl, struct uartlite, uart);

	reg_status = io_read32(uartl->RegBaseAddress + UART_STATUS_OFFSET);
	if (!(reg_status & (UART_SR_RX_FIFO_VALID | UART_SR_RX_FIFO_FULL)))
		return;

	if (uart_ctx->rx_pos < UART_BUF_SIZE - 1)
		uart_ctx->rx_buf[uart_ctx->rx_pos++] = (u8)io_read32(uartl->RegBaseAddress + UART_RX_FIFO_OFFSET);
	else
		io_read32(uartl->RegBaseAddress + UART_RX_FIFO_OFFSET);

	itimer_stop(&uart_ctx->idle_timer);
	itimer_start(&uart_ctx->idle_timer);
}

int uart_init(struct uartlite *uart, u16 uart_id, int irq, uint32_t idle_ticks)
{
	int ret;

	if (!uart)
		return -ENODEV;

	if (XUartLite_Initialize(&uart->uart, uart_id) != XST_SUCCESS)
		return -EFAULT;

	if (XUartLite_SelfTest(&uart->uart) != XST_SUCCESS)
		return -EFAULT;

	ret = request_irq(irq, uart_isr, &uart->uart);
	if (ret)
		return ret;

	uart->rx_pos = 0;
	itimer_node_init(&uart->idle_timer, idle_ticks, false, uart_idle_cb);
	io_write32(uart->uart.RegBaseAddress + UART_CTRL_OFFSET, UART_IRQ_ENABLE);

	return 0;
}

void uart_rxfifo_reset(struct uartlite *uart)
{
	u32 key;
	key = mfmsr();
	microblaze_disable_interrupts();
	uart->rx_pos = 0;
	compiler_barrier();
	mtmsr(key);
}

void uart_poll_out(struct uartlite *uart, const u8 *buf, size_t size)
{
	u32 data;
	size_t i;

	for (i = 0; i < size; i++) {
		data = buf[i];
		while (!(io_read32(uart->uart.RegBaseAddress + UART_STATUS_OFFSET) & UART_SR_TX_FIFO_EMPTY));
		io_write32(uart->uart.RegBaseAddress + UART_TX_FIFO_OFFSET, data);
	}
}
