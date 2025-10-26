#ifndef AGAVE_KEYBOARD_H
#define AGAVE_KEYBOARD_H

#include <agave/kdriver.h>
#include <agave/io.h>
#include <stdbool.h>

#define KEYBOARD_DATA 0x60

#define LEFT_SHIFT_SCANCODE 0x2A
#define RIGHT_SHIFT_SCANCODE 0x36
#define CAPS_LOCK_SCANCODE 0x3A

#define LEFT_ARROW_SCANCODE 0x4B
#define RIGHT_ARROW_SCANCODE 0x4D
#define UP_ARROW_SCANCODE 0x48
#define DOWN_ARROW_SCANCODE 0x50

#define UPPERCASE_OFFSET 32

typedef struct {
    bool shift_pressed;
    bool caps_lock_active;
} keyboard_state_t;

extern keyboard_state_t keyboard_state;

void kb_register(void);

static inline void flush_keyboard_buffer(void) {
    while (inb(0x64) & 0x01) {
        inb(KEYBOARD_DATA);
    }
}


#endif // AGAVE_KEYBOARD_H