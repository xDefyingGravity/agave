#include <agave/pic.h>

static uint8_t pic1_mask = 0xFD;
static uint8_t pic2_mask = 0xFF;

void pic_remap() {
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    pic1_mask = 0xFD;
    pic2_mask = 0xFF;
    outb(PIC1_DATA, pic1_mask);
    outb(PIC2_DATA, pic2_mask);
}

void pic_unmask_irq(uint8_t irq) {
    if (irq < 8) {
        pic1_mask &= ~(1 << irq);
        outb(PIC1_DATA, pic1_mask);
    } else {
        irq -= 8;
        pic2_mask &= ~(1 << irq);
        outb(PIC2_DATA, pic2_mask);
    }
}

void pic_mask_irq(uint8_t irq) {
    if (irq < 8) {
        pic1_mask |= (1 << irq);
        outb(PIC1_DATA, pic1_mask);
    } else {
        irq -= 8;
        pic2_mask |= (1 << irq);
        outb(PIC2_DATA, pic2_mask);
    }
}