#ifndef AGAVE_KCORE_H
#define AGAVE_KCORE_H

#include <stdint.h>
#define PM1a_CNT 0x604
#define SLP_TYPa 0x2000
#define SLP_EN   (1 << 13)

typedef struct {
    uint64_t boot_ticks;
    uint64_t uptime_ticks;
    uint32_t cpus_count;
    const char *kernel_version;
} kcore_information_t;

void kcore_initialize(void);

void kshutdown(void);
void kpanic(const char *fmt, ...);
kcore_information_t* kcore_get_information(void);

#endif // AGAVE_KCORE_H