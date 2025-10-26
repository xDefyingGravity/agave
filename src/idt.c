#include <agave/idt.h>
#include <agave/utils.h>
#include <stdbool.h>

#define IDT_MAX_DESCRIPTORS 256

idt_entry_t idt[IDT_MAX_DESCRIPTORS];
idtr_t idtr;

static bool vectors[IDT_MAX_DESCRIPTORS];
static irq_handler_t irq_handlers[IDT_MAX_DESCRIPTORS];

NORETURN
void exception_handler(void);
void exception_handler() {
    __asm__ volatile(
        "cli\n"
        "hlt\n"
    );
    __builtin_unreachable();
}

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];
    descriptor->isr_low    = (uint32_t)isr & 0xFFFF;
    descriptor->kernel_cs  = 0x08;
    descriptor->attributes = flags;
    descriptor->isr_high   = (uint32_t)isr >> 16;
    descriptor->reserved   = 0;
}

void idt_register_irq(uint8_t irq, irq_handler_t handler) {
    if (irq < IDT_MAX_DESCRIPTORS)
        irq_handlers[irq] = handler;
}

void idt_dispatch_irq(uint8_t irq) {
    if (irq < IDT_MAX_DESCRIPTORS && irq_handlers[irq])
        irq_handlers[irq]();
}
    
extern void* isr_stub_table[];
extern void irq1_handler(void);
extern void irq0_handler(void);

void idt_init() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)(sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1);

    for (uint8_t vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    idt_set_descriptor(0x20, irq0_handler, 0x8E);
    idt_set_descriptor(0x20 + 1, irq1_handler, 0x8E);
    vectors[0x20 + 1] = true;

    __asm__ volatile ("lidt %0" : : "m"(idtr));
}