#ifndef AGAVE_PIC_H
#define AGAVE_PIC_H

#include <agave/io.h>
#include <agave/ports.h>


void pic_remap();

void pic_unmask_irq(uint8_t irq);
void pic_send_eoi(uint8_t irq);

#endif // AGAVE_PIC_H