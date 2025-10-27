#include <agave/time.h>
#include <agave/io.h>
#include <agave/kcore.h>
#include <agave/kcpu.h>
#include <agave/kutils.h>
#include <agave/kvid.h>
#include <agave/pit.h>
#include <agave/ports.h>
#include <stdbool.h>

kcore_information_t kcore_info;

void kshutdown(void) {
  outw(PM1a_CNT, SLP_TYPa | SLP_EN);

  for (volatile int i = 0; i < 100000; i++)
    ;

  __asm__ volatile("lidt (%0)\n"
                   "int3\n"
                   :
                   : "r"(0));

  khalt_cpu(false);
}

void kpanic_ex(const char *file, int line, const char *func, const char *fmt, ...) {
    kdisable_interrupts();
    ksetcolor(kcreate_color(LIGHT_RED, BLACK));
    kclear();

    uint64_t ticks = kcore_get_information()->uptime_ticks;
    uint64_t seconds = ticks / TICK_FREQUENCY;
    uint64_t ms = (ticks % TICK_FREQUENCY) * 1000 / TICK_FREQUENCY;

    uint8_t hours, minutes, seconds_rtc;
    rtc_get_time(&hours, &minutes, &seconds_rtc);

    kprintf("========================================\n");
    kprintf("            KERNEL PANIC\n");
    kprintf("========================================\n\n");

    kprintf("[uptime]  %llu.%03llus\n", seconds, ms);
    kprintf("[rtc]     %02u:%02u:%02u\n\n", hours, minutes, seconds_rtc);

    kprintf("[location] %s() in %s:%d\n\n", func, file, line);

    kprintf("[message]  ");
    va_list args;
    va_start(args, fmt);
    kvprintf(fmt, args);
    va_end(args);
    kprintf("\n\n");

    kprintf("system halted.\n");
    kprintf("========================================\n");

    for (;;) {
        khalt_cpu(false);
    }
}

void kcore_initialize() {
  kcore_info.kernel_version = KERNEL_VERSION;
  kcore_info.cpus_count = kcpu_get_cpu_count();
  kcore_info.boot_ticks = pit_get_ticks();
  kcore_info.uptime_ticks = 0;
}

kcore_information_t *kcore_get_information() {
  kcore_info.uptime_ticks = pit_get_ticks() - kcore_info.boot_ticks;
  return &kcore_info;
}