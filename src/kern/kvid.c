#include <agave/io.h>
#include <agave/kutils.h>
#include <agave/kvid.h>
#include <agave/ports.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

uint16_t *vidptr = (uint16_t *)KVID_POINTER;
size_t krow = 0;
size_t kcol = 0;

uint8_t kcurrent_color = WHITE | (BLACK << 4);
size_t kline_end[SCREEN_HEIGHT] = {0};

void kupdate_cursor(void) {
  uint16_t pos = krow * SCREEN_WIDTH + kcol;

  outb(VGA_CTRL_PORT, VGA_CURSOR_LOW);
  outb(VGA_DATA_PORT, (uint8_t)(pos & 0xFF));

  outb(VGA_CTRL_PORT, VGA_CURSOR_HIGH);
  outb(VGA_DATA_PORT, (uint8_t)((pos >> 8) & 0xFF));
}

void ksetpos(size_t row, size_t col) {
  krow = row;
  kcol = col;
  kupdate_cursor();
}

kpos_t kgetpos(void) {
  kpos_t pos;
  pos.krow = krow;
  pos.kcol = kcol;
  return pos;
}

void ksetcolor(uint8_t color) { kcurrent_color = color; }

void kresetcolor(void) { kcurrent_color = WHITE | (BLACK << 4); }

void kgetcolor(uint8_t *fg, uint8_t *bg) {
  *fg = kcurrent_color & 0x0F;
  *bg = (kcurrent_color >> 4) & 0x0F;
}

void kmove_cursor_left(void) {
  if (kcol > 0) {
    kcol--;
  } else if (krow > 0) {
    krow--;
    kcol = kline_end[krow];
  }
  kupdate_cursor();
}

void kmove_cursor_right(void) {
  if (kcol < kline_end[krow]) {
    kcol++;
  } else if (krow + 1 < SCREEN_HEIGHT) {
    krow++;
    kcol = 0;
  }
  kupdate_cursor();
}

void kmove_cursor_up(void) {
  if (krow > 0) {
    krow--;
    if (kcol > kline_end[krow])
      kcol = kline_end[krow];
  }
  kupdate_cursor();
}

void kmove_cursor_down(void) {
  if (krow + 1 < SCREEN_HEIGHT) {
    krow++;
    if (kcol > kline_end[krow])
      kcol = kline_end[krow];
  }
  kupdate_cursor();
}

void kputchar(char c) {
  if (c == '\n') {
    kline_end[krow] = kcol;
    kcol = 0;
    if (krow + 1 < SCREEN_HEIGHT)
      krow++;
    else {
      for (size_t i = 0; i < (SCREEN_HEIGHT - 1) * SCREEN_WIDTH; i++)
        vidptr[i] = vidptr[i + SCREEN_WIDTH];
      for (size_t i = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH;
           i < SCREEN_HEIGHT * SCREEN_WIDTH; i++)
        vidptr[i] = (kcurrent_color << 8) | ' ';
      for (size_t i = 0; i < SCREEN_HEIGHT - 1; i++)
        kline_end[i] = kline_end[i + 1];
      kline_end[SCREEN_HEIGHT - 1] = 0;
    }
  } else if (c == '\b') {
    if (kcol > 0) {
      kcol--;
      vidptr[krow * SCREEN_WIDTH + kcol] = (kcurrent_color << 8) | ' ';
      if (kline_end[krow] > kcol)
        kline_end[krow] = kcol;
    } else if (krow > 0) {
      krow--;
      kcol = kline_end[krow];
    }
  } else if (c == '\r') {
    kcol = 0;
  } else {
    vidptr[krow * SCREEN_WIDTH + kcol] = (kcurrent_color << 8) | c;
    if (kcol >= kline_end[krow])
      kline_end[krow] = kcol + 1;

    kcol++;
    if (kcol >= SCREEN_WIDTH) {
      kcol = 0;
      if (krow + 1 < SCREEN_HEIGHT)
        krow++;
      else {

        for (size_t i = 0; i < (SCREEN_HEIGHT - 1) * SCREEN_WIDTH; i++)
          vidptr[i] = vidptr[i + SCREEN_WIDTH];
        for (size_t i = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH;
             i < SCREEN_HEIGHT * SCREEN_WIDTH; i++)
          vidptr[i] = (kcurrent_color << 8) | ' ';
        for (size_t i = 0; i < SCREEN_HEIGHT - 1; i++)
          kline_end[i] = kline_end[i + 1];
        kline_end[SCREEN_HEIGHT - 1] = 0;
      }
    }
  }

  kupdate_cursor();
}

void kclear(void) {
  for (size_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    vidptr[i] = (0x07 << 8) | ' ';
  for (size_t i = 0; i < SCREEN_HEIGHT; i++)
    kline_end[i] = 0;
  krow = 0;
  kcol = 0;
}

void kprint(const char *str) {
  for (size_t i = 0; str[i] != '\0'; i++)
    kputchar(str[i]);
}

void kvprintf(const char *fmt, va_list args) {
    char buffer[64];

    for (size_t i = 0; fmt[i]; i++) {
        if (fmt[i] != '%') {
            kputchar(fmt[i]);
            continue;
        }

        i++;
        bool zero_pad = false;
        size_t width = 0;

        if (fmt[i] == '0') {
            zero_pad = true;
            i++;
        }

        while (fmt[i] >= '0' && fmt[i] <= '9') {
            width = width * 10 + (fmt[i] - '0');
            i++;
        }

        bool long_flag = false;
        bool long_long_flag = false;

        if (fmt[i] == 'l') {
            i++;
            if (fmt[i] == 'l') {
                long_long_flag = true;
                i++;
            } else {
                long_flag = true;
            }
        }

        switch (fmt[i]) {
        case 'c':
            kputchar((char)va_arg(args, int));
            break;

        case 's': {
            const char *s = va_arg(args, const char *);
            if (!s) s = "(null)";
            size_t len = strlen(s);
            for (size_t j = 0; j + len < width; j++)
                kputchar(zero_pad ? '0' : ' ');
            kprint(s);
            break;
        }

        case 'd':
        case 'i': {
            long long val;
            if (long_long_flag)
                val = va_arg(args, long long);
            else if (long_flag)
                val = va_arg(args, long);
            else
                val = va_arg(args, int);
            kitoa(val, buffer, 10);
            size_t len = strlen(buffer);
            for (size_t j = 0; j + len < width; j++)
                kputchar(zero_pad ? '0' : ' ');
            kprint(buffer);
            break;
        }

        case 'u': {
            unsigned long long val;
            if (long_long_flag)
                val = va_arg(args, unsigned long long);
            else if (long_flag)
                val = va_arg(args, unsigned long);
            else
                val = va_arg(args, unsigned int);
            kitoa_unsigned(val, buffer, 10);
            size_t len = strlen(buffer);
            for (size_t j = 0; j + len < width; j++)
                kputchar(zero_pad ? '0' : ' ');
            kprint(buffer);
            break;
        }

        case 'x': {
            unsigned long long val;
            if (long_long_flag)
                val = va_arg(args, unsigned long long);
            else if (long_flag)
                val = va_arg(args, unsigned long);
            else
                val = va_arg(args, unsigned int);
            kitoa_unsigned(val, buffer, 16);
            size_t len = strlen(buffer);
            for (size_t j = 0; j + len < width; j++)
                kputchar(zero_pad ? '0' : ' ');
            kprint(buffer);
            break;
        }

        case 'p': {
            uintptr_t ptr = (uintptr_t)va_arg(args, void *);
            kprint("0x");
            kitoa_unsigned(ptr, buffer, 16);
            kprint(buffer);
            break;
        }

        case '%':
            kputchar('%');
            break;

        default:
            kputchar('%');
            kputchar(fmt[i]);
            break;
        }
    }
}

void kprintf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  kvprintf(fmt, args);
  va_end(args);
}