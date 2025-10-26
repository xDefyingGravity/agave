#ifndef AGAVE_IDT_H
#define AGAVE_IDT_H

#include <stdint.h>
#include <agave/utils.h>

#define IRQ1 0x21
#define IDT_FLAG_PRESENT 0x80
#define IDT_FLAG_INTERRUPT 0x0E

typedef struct {
	uint16_t    isr_low;
	uint16_t    kernel_cs;
	uint8_t     reserved;
	uint8_t     attributes;
	uint16_t    isr_high;
} PACKED idt_entry_t;

typedef struct {
	uint16_t	limit;
	uint32_t	base;
} PACKED idtr_t;

typedef void (*irq_handler_t)(void);

ALIGNED((0x10))
extern idt_entry_t idt[256];
extern idtr_t idtr;

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);
void idt_register_irq(uint8_t irq, irq_handler_t handler);
void idt_dispatch_irq(uint8_t irq);
void idt_init(void);

#endif // AGAVE_IDT_H