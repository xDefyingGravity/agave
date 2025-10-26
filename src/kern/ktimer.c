#include <agave/pit.h>
#include <agave/ktimer.h>
#include <stdint.h>

static uint32_t timer_frequency_hz = 100;
void ktimer_initialize(uint32_t frequency_hz) {
    timer_frequency_hz = frequency_hz;
    pit_init(frequency_hz);
}

uint64_t ktimer_get_ticks(void) {
    return pit_get_ticks();
}

uint64_t ktimer_get_milliseconds(void) {
    return (ktimer_get_ticks() * 1000) / timer_frequency_hz;
}

uint64_t ktimer_get_microseconds(void) {
    return (ktimer_get_ticks() * 1000000) / timer_frequency_hz;
}

uint64_t ktimer_get_nanoseconds(void) {
    return (ktimer_get_ticks() * 1000000000) / timer_frequency_hz;
}

void ksleep(uint32_t milliseconds) {
    uint64_t start_ticks = ktimer_get_ticks();
    uint64_t wait_ticks = (milliseconds * timer_frequency_hz) / 1000;
    while (ktimer_get_ticks() - start_ticks < wait_ticks) {
        __asm__ volatile("hlt");
    }
}

void ksleep_micro(uint32_t microseconds) {
    uint64_t start_ticks = ktimer_get_ticks();
    uint64_t wait_ticks = (microseconds * timer_frequency_hz) / 1000000;
    while (ktimer_get_ticks() - start_ticks < wait_ticks) {
        __asm__ volatile("hlt");
    }
}

void ksleep_nano(uint32_t nanoseconds) {
    uint64_t start_ticks = ktimer_get_ticks();
    uint64_t wait_ticks = (nanoseconds * timer_frequency_hz) / 1000000000;
    while (ktimer_get_ticks() - start_ticks < wait_ticks) {
        __asm__ volatile("hlt");
    }
}

void ktimer_wait_ticks(uint64_t ticks) {
    uint64_t start_ticks = ktimer_get_ticks();
    while (ktimer_get_ticks() - start_ticks < ticks) {
        __asm__ volatile("hlt");
    }
}