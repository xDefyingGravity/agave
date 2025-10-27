#include <agave/io.h>
#include <agave/time.h>

static uint8_t rtc_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

void rtc_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
    while (rtc_read(0x0A) & 0x80);

    *seconds = rtc_read(0x00);
    *minutes = rtc_read(0x02);
    *hours   = rtc_read(0x04);

    uint8_t status = rtc_read(0x0B);
    if (!(status & 0x04)) {
        *seconds = ((*seconds >> 4) * 10) + (*seconds & 0x0F);
        *minutes = ((*minutes >> 4) * 10) + (*minutes & 0x0F);
        *hours   = ((*hours >> 4) * 10) + (*hours & 0x0F);
    }
}