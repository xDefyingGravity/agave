#include <agave/pit.h>
#include <agave/pic.h>
#include <agave/ports.h>
#include <agave/idt.h>
#include <agave/io.h>
#include <stdint.h>

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_FREQUENCY 1193182

static volatile uint64_t pit_ticks = 0;

void pit_callback(void) {
    pit_ticks++;
    pic_send_eoi(0);
}

CREATE_ISR(0, pit_callback)

void pit_init(uint32_t frequency_hz) {
    uint16_t divisor = PIT_FREQUENCY / frequency_hz;

    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8));
    idt_set_descriptor(IRQ0, irq0_handler, IDT_FLAG_PRESENT | IDT_FLAG_INTERRUPT);
    pic_unmask_irq(0);
}

uint64_t pit_get_ticks(void) {
    return pit_ticks;
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}