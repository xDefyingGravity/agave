#ifndef AGAVE_TIME_H
#define AGAVE_TIME_H

#include <stdint.h>
#include <stddef.h>

typedef struct time time_t;
typedef struct date date_t;
typedef struct datetime datetime_t;

struct time {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

struct date {
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

struct datetime {
    time_t time;
    date_t date;
};

void time_get_current(time_t *t);
void date_get_current(date_t *d);
void datetime_get_current(datetime_t *dt);
void time_to_string(const time_t *t, char *buffer, size_t buffer_size);
void date_to_string(const date_t *d, char *buffer, size_t buffer_size);
void datetime_to_string(const datetime_t *dt, char *buffer, size_t buffer_size);


#endif // AGAVE_TIME_H