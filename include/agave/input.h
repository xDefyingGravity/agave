#ifndef AGAVE_INPUT_H
#define AGAVE_INPUT_H

#include <stdint.h>
#include <stdbool.h>
#include <agave/drivers/keyboard.h>

#define MAX_INPUT_HOOKS 16

/**
 * input_hook_t
 * on_key_press: Function pointer called when a key is pressed. Should return true if the key event is handled and should not be processed further.
 * on_key_release: Function pointer called when a key is released.
 */
typedef struct {
    bool (*on_key_press)(uint8_t c);
    void (*on_key_release)(uint8_t c);
} input_hook_t;

extern input_hook_t input_hooks[MAX_INPUT_HOOKS];

void on_key_press(uint8_t c);
void on_key_release(uint8_t c);

void register_input_hook(input_hook_t hook);
void unregister_input_hook(input_hook_t hook);

#define REGISTER_INPUT_HOOK(key_press, key_release) \
    do { \
        input_hook_t hook; \
        hook.on_key_press = key_press; \
        hook.on_key_release = key_release; \
        register_input_hook(hook); \
    } while (0);

#define UNREGISTER_INPUT_HOOK(key_press, key_release) \
    do { \
        input_hook_t hook; \
        hook.on_key_press = key_press; \
        hook.on_key_release = key_release; \
        unregister_input_hook(hook); \
    } while (0);

#endif // AGAVE_INPUT_H