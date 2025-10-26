#ifndef AGAVE_KUTILS_H
#define AGAVE_KUTILS_H

#define inf_idle() while (1) __asm__ volatile("hlt");

char* kitoa(int value, char* buffer, int base);
char* kitoa_unsigned(unsigned int value, char* buffer, int base);

static inline void kenable_interrupts() {
    __asm__ volatile("sti");
}

static inline void kdisable_interrupts() {
    __asm__ volatile("cli");
}

#endif // AGAVE_KUTILS_H