#include "agave/input.h"
#include "agave/ports.h"
#include <agave/drivers/keyboard.h>
#include <agave/idt.h>
#include <agave/io.h>
#include <agave/kdriver.h>
#include <agave/keys.h>
#include <agave/kvid.h>
#include <agave/pic.h>
#include <agave/utils.h>
#include <stdbool.h>

keyboard_state_t keyboard_state = {false, false};

static const uint8_t scancode_map[128] = {
      KEY_NONE,  KEY_ESC,   '1',           '2',
      '3',       '4',       '5',           '6', // 0x00 - 0x07
      '7',       '8',       '9',           '0',
      '-',       '=',       KEY_BACKSPACE, KEY_TAB, // 0x08 - 0x0F
      'q',       'w',       'e',           'r',
      't',       'y',       'u',           'i', // 0x10 - 0x17
      'o',       'p',       '[',           ']',
      KEY_ENTER, KEY_NONE,  'a',           's', // 0x18 - 0x1F
      'd',       'f',       'g',           'h',
      'j',       'k',       'l',           ';', // 0x20 - 0x27
      '\'',      '`',       KEY_NONE,      '\\',
      'z',       'x',       'c',           'v', // 0x28 - 0x2F
      'b',       'n',       'm',           ',',
      '.',       '/',       KEY_NONE,      '*', // 0x30 - 0x37
      KEY_NONE,  KEY_SPACE, KEY_NONE,      KEY_NONE,
};

NAKED void keyboard_isr(void) {
  __asm__ volatile("pusha\n\t"
                   "call keyboard_handler\n\t"
                   "popa\n\t"
                   "movb $0x20, %%al\n\t"
                   "outb %%al, $0x20\n\t"
                   "iret"
                   :
                   :
                   : "al");
}

static uint8_t scancode_to_ascii(uint8_t scancode) {
  return (scancode < sizeof(scancode_map)) ? scancode_map[scancode] : KEY_NONE;
}

static const char shifted_table[256] = {
    ['1'] = '!',  ['2'] = '@', ['3'] = '#', ['4'] = '$', ['5'] = '%',
    ['6'] = '^',  ['7'] = '&', ['8'] = '*', ['9'] = '(', ['0'] = ')',
    ['-'] = '_',  ['='] = '+', ['['] = '{', [']'] = '}', [';'] = ':',
    ['\''] = '"', [','] = '<', ['.'] = '>', ['/'] = '?', ['`'] = '~',
    ['\\'] = '|'};

void keyboard_handler(void) {
    static bool extended = false;
    uint8_t scancode = inb(KEYBOARD_DATA);
    bool released = scancode & 0x80;
    uint8_t keycode = scancode & 0x7F;

    if (scancode == 0xE0) {
        extended = true;
        outb(PIC1_COMMAND, PIC_EOI);
        return;
    }

    bool is_shift = (keycode == LEFT_SHIFT_SCANCODE || keycode == RIGHT_SHIFT_SCANCODE);
    keyboard_state.shift_pressed = (keyboard_state.shift_pressed & !is_shift) | (is_shift & !released);

    if (keycode == CAPS_LOCK_SCANCODE && !released)
        keyboard_state.caps_lock_active ^= true;

    uint8_t ascii = KEY_NONE;

    if (extended) {
        switch (keycode) {
            case UP_ARROW_SCANCODE: ascii = KEY_UP; break;
            case LEFT_ARROW_SCANCODE: ascii = KEY_LEFT; break;
            case RIGHT_ARROW_SCANCODE: ascii = KEY_RIGHT; break;
            case DOWN_ARROW_SCANCODE: ascii = KEY_DOWN; break;
            default: ascii = KEY_NONE; break;
        }
        extended = false;
    } else {
        ascii = scancode_to_ascii(keycode);
        if (ascii >= ' ') {
            bool is_alpha = (ascii >= 'a' && ascii <= 'z');
            bool upper = keyboard_state.shift_pressed ^ (keyboard_state.caps_lock_active && is_alpha);
            if (upper)
                ascii = is_alpha ? ascii - UPPERCASE_OFFSET : shifted_table[ascii];
        }
    }

    if (ascii != KEY_NONE) {
        if (!released)
            on_key_press(ascii);
        else
            on_key_release(ascii);
    }

    outb(PIC1_COMMAND, PIC_EOI);
    pic_unmask_irq(1);
}

static int kb_init(void) {
  kprint("[info] keyboard driver initialized\n");
  idt_set_descriptor(IRQ1, keyboard_isr, IDT_FLAG_PRESENT | IDT_FLAG_INTERRUPT);
  return 0;
}

KDRIVER_REGISTER(keyboard, kb_init)