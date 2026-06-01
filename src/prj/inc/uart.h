#pragma once
#include "xil_types.h"
#include "define.h"
#include "msgq.h"
#include "xuartlite.h"
#include "itimer.h"

#define UART_BUF_SIZE        (64U)
#define UART_BUF_ALIGN       (4U)
#define UART_BUF_SIZE_CAL    ROUND_UP(UART_BUF_SIZE, UART_BUF_ALIGN)
#define UART_QUEUE_CAPACITY  (3U)

MSGQ_DECLARE(uart_rxq);

struct uartlite {
	u8 rx_buf[UART_BUF_SIZE_CAL];
	size_t rx_pos;
	struct itimer_node idle_timer;
	XUartLite uart;
};

int uart_init(struct uartlite *uart, u16 uart_id, int irq, uint32_t idle_ticks);
void uart_rxfifo_reset(struct uartlite *uart);
void uart_poll_out(struct uartlite *uart, const u8 *buf, size_t size);
