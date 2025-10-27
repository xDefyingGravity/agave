#ifndef AGAVE_TIME_H
#define AGAVE_TIME_H

#include <stdint.h>

void rtc_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);

#endif // AGAVE_TIME_H