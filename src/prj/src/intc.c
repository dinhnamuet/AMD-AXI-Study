#include "xparameters.h"
#include "intc.h"
#include "io.h"
#include "gcc.h"
#include "define.h"
#include "mb_interface.h"

#define IMR_OFFSET		    0x20U
#define IVAR_OFFSET		    0x100U
#define IVAR_END_OFFSET		0x174U
#define NUM_RESERVED		(((IVAR_OFFSET - IMR_OFFSET) / sizeof(u32)) - 1)
#define NUM_IRQ			    ((IVAR_END_OFFSET - IVAR_OFFSET) / sizeof(u32))

#define MER_ME			    BIT(0)
#define MER_HIE			    BIT(1)

struct irq_controller {
	volatile u32 ISR;
	volatile u32 IPR;
	volatile u32 IER;
	volatile u32 IAR;
	volatile u32 SIE;
	volatile u32 CIE;
	volatile u32 IVR;
	volatile u32 MER;
	volatile u32 IMR;
	volatile u32 pad[NUM_RESERVED];
	volatile u32 IVAR[NUM_IRQ];
} __aligned(32);

struct isr_entry {
	irq_handler_t handler;
	void *arg;
};

static struct irq_controller *intc;
static struct isr_entry isr_table[NUM_IRQ];

int intc_init_once(UINTPTR intc_base)
{
	if (intc)
		return -EEXIST;

	intc = (struct irq_controller *)intc_base;
	io_write32((UINTPTR)&intc->MER, 0);
	io_write32((UINTPTR)&intc->IER, 0);
	io_write32((UINTPTR)&intc->IAR, 0xFFFFFFFFU);
	io_write32((UINTPTR)&intc->IMR, 0);
	io_write32((UINTPTR)&intc->MER, MER_ME | MER_HIE);
	microblaze_enable_interrupts();

	return 0;
}

int request_irq(int irq, irq_handler_t handler, void *arg)
{
	u32 ier;

	if (!intc)
		return -ENODEV;

	if (irq < 0 || irq >= (int)NUM_IRQ || !handler)
		return -EINVAL;

	if (isr_table[irq].handler)
		return -EEXIST;

	isr_table[irq].handler = handler;
	isr_table[irq].arg = arg;

	ier = io_read32((UINTPTR)&intc->IER);
	io_write32((UINTPTR)&intc->IER, ier | BIT(irq));

	return 0;
}

int free_irq(int irq)
{
	u32 ier;

	if (!intc)
		return -ENODEV;

	if (irq < 0 || irq >= (int)NUM_IRQ)
		return -EINVAL;

	ier = io_read32((UINTPTR)&intc->IER);
	io_write32((UINTPTR)&intc->IER, ier & ~BIT(irq));
	io_write32((UINTPTR)&intc->IAR, BIT(irq));

	isr_table[irq].handler = NULL;
	isr_table[irq].arg = NULL;

	return 0;
}

int request_fast_irq(int irq, fast_irq_handler_t handler)
{
	u32 imr;
	u32 ier;

#if defined(XPAR_MICROBLAZE_0_AXI_INTC_HAS_FAST) && XPAR_MICROBLAZE_0_AXI_INTC_HAS_FAST != 1
	return -1;
#endif

	if (!intc)
		return -ENODEV;

	if (irq < 0 || irq >= (int)NUM_IRQ || !handler)
		return -EINVAL;

	ier = io_read32((UINTPTR)&intc->IER);
	io_write32((UINTPTR)&intc->IER, ier & ~BIT(irq));

	io_write32((UINTPTR)&intc->IVAR[irq], (u32)(UINTPTR)handler);

	imr = io_read32((UINTPTR)&intc->IMR);
	io_write32((UINTPTR)&intc->IMR, imr | BIT(irq));

	io_write32((UINTPTR)&intc->IER, ier | BIT(irq));

	return 0;
}

int free_fast_irq(int irq)
{
	u32 imr;
	u32 ier;

#if defined(XPAR_MICROBLAZE_0_AXI_INTC_HAS_FAST) && XPAR_MICROBLAZE_0_AXI_INTC_HAS_FAST != 1
	return -1;
#endif

	if (!intc)
		return -ENODEV;

	if (irq < 0 || irq >= (int)NUM_IRQ)
		return -EINVAL;

	ier = io_read32((UINTPTR)&intc->IER);
	io_write32((UINTPTR)&intc->IER, ier & ~BIT(irq));
	io_write32((UINTPTR)&intc->IAR, 1U << irq);

	imr = io_read32((UINTPTR)&intc->IMR);
	io_write32((UINTPTR)&intc->IMR, imr & ~BIT(irq));

	io_write32((UINTPTR)&intc->IVAR[irq], 0);

	return 0;
}

void __attribute__((interrupt_handler)) _do_irq(void)
{
	u32 ivr;
	struct isr_entry *entry;

	ivr = io_read32((UINTPTR)&intc->IVR);
	if (ivr >= NUM_IRQ)
		return;

	entry = &isr_table[ivr];
	if (entry->handler)
		entry->handler(entry->arg);

	io_write32((UINTPTR)&intc->IAR, 1U << ivr);
}
