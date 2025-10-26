#ifndef AGAVE_KVID_H
#define AGAVE_KVID_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define KVID_POINTER 0xb8000

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define _VGA_BLACK         0x0
#define _VGA_BLUE          0x1
#define _VGA_GREEN         0x2
#define _VGA_CYAN          0x3
#define _VGA_RED           0x4
#define _VGA_MAGENTA       0x5
#define _VGA_BROWN         0x6
#define _VGA_LIGHT_GRAY    0x7
#define _VGA_DARK_GRAY     0x8
#define _VGA_LIGHT_BLUE    0x9
#define _VGA_LIGHT_GREEN   0xA
#define _VGA_LIGHT_CYAN    0xB
#define _VGA_LIGHT_RED     0xC
#define _VGA_LIGHT_MAGENTA 0xD
#define _VGA_YELLOW        0xE
#define _VGA_WHITE         0xF

typedef enum kcolor {
    BLACK = _VGA_BLACK,
    BLUE = _VGA_BLUE,
    GREEN = _VGA_GREEN,
    CYAN = _VGA_CYAN,
    RED = _VGA_RED,
    MAGENTA = _VGA_MAGENTA,
    BROWN = _VGA_BROWN,
    LIGHT_GRAY = _VGA_LIGHT_GRAY,
    DARK_GRAY = _VGA_DARK_GRAY,
    LIGHT_BLUE = _VGA_LIGHT_BLUE,
    LIGHT_GREEN = _VGA_LIGHT_GREEN,
    LIGHT_CYAN = _VGA_LIGHT_CYAN,
    LIGHT_RED = _VGA_LIGHT_RED,
    LIGHT_MAGENTA = _VGA_LIGHT_MAGENTA,
    YELLOW = _VGA_YELLOW,
    WHITE = _VGA_WHITE,
} kcolor_t;

static inline uint8_t kcreate_color(kcolor_t fg, kcolor_t bg) {
    return fg | (bg << 4);
}

extern uint16_t* vidptr;

extern uint8_t kcurrent_color;
extern size_t kline_end[SCREEN_HEIGHT];

typedef struct kpos {
    size_t krow;
    size_t kcol;
} kpos_t;

void ksetpos(size_t row, size_t col);
kpos_t kgetpos(void);

void kputchar(char c);
void kprint(const char* str);
void kprintf(const char* fmt, ...);

void ksetcolor(uint8_t color);
void kresetcolor(void);
void kgetcolor(uint8_t* fg, uint8_t* bg);

void kmove_cursor_left(void);
void kmove_cursor_right(void);
void kmove_cursor_up(void);
void kmove_cursor_down(void);

void kvprintf(const char* fmt, va_list args);

void kupdate_cursor(void);

void kclear(void);
#endif //AGAVE_KVID_H