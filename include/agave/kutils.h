#ifndef AGAVE_KUTILS_H
#define AGAVE_KUTILS_H

#include "stdbool.h"
#define kidle() while (1) { khalt_cpu(false); }

char* kitoa(int value, char* buffer, int base);
char* kitoa_unsigned(unsigned int value, char* buffer, int base);

static inline void kenable_interrupts() {
    __asm__ volatile("sti");
}

static inline void kdisable_interrupts() {
    __asm__ volatile("cli");
}

static inline void khalt_cpu(bool disable_interrupts) {
    if (disable_interrupts) {
        kdisable_interrupts();
    }
    __asm__ volatile("hlt");
}



#endif // AGAVE_KUTILS_H