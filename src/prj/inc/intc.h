#pragma once
#include "xil_types.h"

typedef void (*irq_handler_t)(void *);
typedef void (*fast_irq_handler_t)(void);

#ifndef IRQ_FAST_DECLARE
#define IRQ_FAST_DECLARE(name) \
	void __attribute__ ((fast_interrupt)) name(void)
#endif

int intc_init_once(UINTPTR intc_base);
int request_irq(int irq, irq_handler_t handler, void *arg);
int free_irq(int irq);
int request_fast_irq(int irq, fast_irq_handler_t handler);
int free_fast_irq(int irq);
