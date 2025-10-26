#include <agave/input.h>
#include <agave/kvid.h>
#include <agave/keys.h>

input_hook_t input_hooks[MAX_INPUT_HOOKS] = {0};
void on_key_press(uint8_t c) {
    for (int i = 0; i < MAX_INPUT_HOOKS; i++) {
        if (input_hooks[i].on_key_press && input_hooks[i].on_key_press(c)) {
            return;
        }
    }

    if (c == KEY_LEFT) {
        kmove_cursor_left();
        return;
    } else if (c == KEY_RIGHT) {
        kmove_cursor_right();
        return;
    } else if (c == KEY_UP) {
        kmove_cursor_up();
        return;
    } else if (c == KEY_DOWN) {
        kmove_cursor_down();
        return;
    }

    kputchar(c);
}

void on_key_release(uint8_t c) {
    for (int i = 0; i < MAX_INPUT_HOOKS; i++) {
        if (input_hooks[i].on_key_release) {
            input_hooks[i].on_key_release(c);
        }
    }
    
    (void)c;
    return;
}

void register_input_hook(input_hook_t hook) {
    int free_slot = -1;

    for (int i = 0; i < MAX_INPUT_HOOKS; i++) {
        if (input_hooks[i].on_key_press == hook.on_key_press &&
            input_hooks[i].on_key_release == hook.on_key_release) {
            return;
        }
        if (free_slot == -1 && input_hooks[i].on_key_press == NULL &&
            input_hooks[i].on_key_release == NULL) {
            free_slot = i;
        }
    }

    if (free_slot != -1) {
        input_hooks[free_slot] = hook;
    } else {
        kprintf("[warn] unable to register input hook, no available slots.\n");
    }
}

void unregister_input_hook(input_hook_t hook) {
    for (int i = 0; i < MAX_INPUT_HOOKS; i++) {
        if (input_hooks[i].on_key_press == hook.on_key_press &&
            input_hooks[i].on_key_release == hook.on_key_release) {
            input_hooks[i].on_key_press = NULL;
            input_hooks[i].on_key_release = NULL;
            return;
        }
    }
}