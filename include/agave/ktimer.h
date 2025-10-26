#ifndef AGAVE_KTIMER_H
#define AGAVE_KTIMER_H

#include <agave/pit.h>

void ktimer_initialize(uint32_t frequency_hz);
uint64_t ktimer_get_ticks(void);
uint64_t ktimer_get_milliseconds(void);
uint64_t ktimer_get_microseconds(void);
uint64_t ktimer_get_nanoseconds(void);
void ksleep(uint32_t milliseconds);
void ksleep_micro(uint32_t microseconds);
void ksleep_nano(uint32_t nanoseconds);
void ktimer_wait_ticks(uint64_t ticks);

#endif // AGAVE_KTIMER_H