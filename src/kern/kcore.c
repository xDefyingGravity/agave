#include <agave/kcpu.h>
#include <agave/kutils.h>
#include "agave/pit.h"
#include <agave/kcore.h>
#include <agave/ports.h>
#include <agave/io.h>
#include <agave/kvid.h>

kcore_information_t kcore_info;

static inline void khalt_cpu(void) {
    for (;;) __asm__ volatile("hlt");
}

void kshutdown(void) {
    outw(PM1a_CNT, SLP_TYPa | SLP_EN);

    for (volatile int i = 0; i < 100000; i++);

    __asm__ volatile(
        "lidt (%0)\n" 
        "int3\n"
        :
        : "r"(0)
    );

    khalt_cpu();
}


void kpanic(const char *fmt, ...) {
    kdisable_interrupts();

    ksetcolor(kcreate_color(LIGHT_RED, BLACK));
    kclear();

    kprint("KERNEL PANIC\n");
    kprint("============\n\n");

    va_list args;
    va_start(args, fmt);
    kvprintf(fmt, args);
    va_end(args);

    kprint("\n\nsystem halted.");

    for (;;) {
        khalt_cpu();
    }
}

void kcore_initialize() {
    kcore_info.kernel_version = KERNEL_VERSION;
    kcore_info.cpus_count = kcpu_get_cpu_count();
    kcore_info.boot_ticks = pit_get_ticks();
    kcore_info.uptime_ticks = 0;
}

kcore_information_t* kcore_get_information() {
    kcore_info.uptime_ticks = pit_get_ticks() - kcore_info.boot_ticks;
    return &kcore_info;
}