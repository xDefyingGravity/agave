#include <agave/kvid.h>
#include <agave/io.h>
#include <agave/time.h>

#ifndef TIMEZONE
#define TIMEZONE -4 
#endif

static uint8_t rtc_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static void rtc_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
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

static void rtc_get_date(uint8_t *day, uint8_t *month, uint16_t *year) {
    while (rtc_read(0x0A) & 0x80);

    *day   = rtc_read(0x07);
    *month = rtc_read(0x08);
    uint8_t year_low = rtc_read(0x09);

    uint8_t status = rtc_read(0x0B);
    if (!(status & 0x04)) {
        *day   = ((*day >> 4) * 10) + (*day & 0x0F);
        *month = ((*month >> 4) * 10) + (*month & 0x0F);
        year_low = ((year_low >> 4) * 10) + (year_low & 0x0F);
    }

    *year = 2000 + year_low;
}

void time_get_current(time_t *t) {
    rtc_get_time(&t->hours, &t->minutes, &t->seconds);
}

void date_get_current(date_t *d) {
    rtc_get_date(&d->day, &d->month, &d->year);
}

static void adjust_date(datetime_t *dt, int delta_days) {
    static const uint8_t days_in_month[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    dt->date.day += delta_days;
    while (dt->date.day < 1) {
        dt->date.month--;
        if (dt->date.month < 1) {
            dt->date.month = 12;
            dt->date.year--;
        }
        dt->date.day += days_in_month[dt->date.month - 1];
    }
    while (dt->date.day > days_in_month[dt->date.month - 1]) {
        dt->date.day -= days_in_month[dt->date.month - 1];
        dt->date.month++;
        if (dt->date.month > 12) {
            dt->date.month = 1;
            dt->date.year++;
        }
    }
}

void datetime_get_current(datetime_t *dt) {
    time_get_current(&dt->time);
    date_get_current(&dt->date);

    int adjusted_hours = (int)dt->time.hours + TIMEZONE;
    int delta_days = 0;

    if (adjusted_hours < 0) {
        adjusted_hours += 24;
        delta_days = -1;
    } else if (adjusted_hours >= 24) {
        adjusted_hours -= 24;
        delta_days = 1;
    }

    dt->time.hours = (uint8_t)adjusted_hours;
    if (delta_days != 0) adjust_date(dt, delta_days);
}

void time_to_string(const time_t *t, char *buffer, size_t buffer_size) {
    ksnprintf(buffer, buffer_size, "%02u:%02u:%02u", t->hours, t->minutes, t->seconds);
}

void date_to_string(const date_t *d, char *buffer, size_t buffer_size) {
    ksnprintf(buffer, buffer_size, "%02u/%02u/%04u", d->day, d->month, d->year);
}

void datetime_to_string(const datetime_t *dt, char *buffer, size_t buffer_size) {
    ksnprintf(buffer, buffer_size, "%02u/%02u/%04u %02u:%02u:%02u",
              dt->date.day, dt->date.month, dt->date.year,
              dt->time.hours, dt->time.minutes, dt->time.seconds);
}