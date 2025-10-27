#ifndef AGAVE_IDT_H
#define AGAVE_IDT_H

#include <stdint.h>
#include <agave/utils.h>

#define IDT_MAX_DESCRIPTORS 256
#define IRQ_BASE 0x20
#define IRQ_COUNT 16

#define IDT_FLAG_PRESENT 0x80
#define IDT_FLAG_INTERRUPT 0x0E

#define IRQ0 0x20
#define IRQ1 0x21
#define IRQ2 0x22
#define IRQ3 0x23
#define IRQ4 0x24
#define IRQ5 0x25
#define IRQ6 0x26
#define IRQ7 0x27
#define IRQ8 0x28

#define IRQ9 0x29
#define IRQ10 0x2A
#define IRQ11 0x2B
#define IRQ12 0x2C
#define IRQ13 0x2D
#define IRQ14 0x2E
#define IRQ15 0x2F


#define CREATE_ISR(num, irq_handler) \
NAKED void irq##num##_handler(void) { \
    __asm__ volatile( \
        "pusha\n\t" \
        "call " #irq_handler "\n\t" \
        "popa\n\t" \
        "iret" \
    ); \
}

typedef struct {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t reserved;
    uint8_t attributes;
    uint16_t isr_high;
} PACKED idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} PACKED idtr_t;

typedef void (*irq_handler_t)(void);

ALIGNED(16)
extern idt_entry_t idt[IDT_MAX_DESCRIPTORS];
extern idtr_t idtr;

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);
void idt_init(void);

void register_irq(uint8_t irq, irq_handler_t handler);
void irq_dispatch(uint32_t irq);

#endif // AGAVE_IDT_H