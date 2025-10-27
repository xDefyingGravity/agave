#include "agave/kutils.h"
#include <agave/idt.h>
#include <agave/utils.h>

static irq_handler_t irq_handlers[IRQ_COUNT] = {0};
idt_entry_t idt[IDT_MAX_DESCRIPTORS];
idtr_t idtr;

NORETURN void exception_handler(void) {
    khalt_cpu(true);
    __builtin_unreachable();
}

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* desc = &idt[vector];
    desc->isr_low    = (uint32_t)isr & 0xFFFF;
    desc->kernel_cs  = 0x08;
    desc->attributes = flags;
    desc->isr_high   = (uint32_t)isr >> 16;
    desc->reserved   = 0;
}

void register_irq(uint8_t irq, irq_handler_t handler) {
    if (irq < IRQ_COUNT) irq_handlers[irq] = handler;
}

void irq_dispatch(uint32_t irq) {
    if (irq < IRQ_COUNT && irq_handlers[irq])
        irq_handlers[irq]();
}

extern void (*isr_stub_table[])(void);

void idt_init(void) {
    idtr.base  = (uintptr_t)&idt[0];
    idtr.limit = sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    for (uint8_t i = 0; i < 32; i++)
        idt_set_descriptor(i, &((void**)isr_stub_table)[i], 0x8E);

    for (uint8_t i = 0; i < IRQ_COUNT; i++)
        idt_set_descriptor(IRQ_BASE + i, &((void**)isr_stub_table)[32 + i], 0x8E);

    __asm__ volatile("lidt %0" : : "m"(idtr));
}