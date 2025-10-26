#ifndef AGAVE_PIT_H
#define AGAVE_PIT_H

#include <stdint.h>

void pit_init(uint32_t frequency_hz);
uint64_t pit_get_ticks(void);

#endif // AGAVE_PIT_H