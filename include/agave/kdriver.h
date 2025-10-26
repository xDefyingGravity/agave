#ifndef AGAVE_KDRIVER_H
#define AGAVE_KDRIVER_H

#include <stdint.h>
#include <stddef.h>

typedef struct kdriver {
    const char* name;
    int (*init)(void);
    void (*handle_interrupt)(void*); 
} kdriver_t;

void kdriver_register(kdriver_t* driver);
void kdriver_init_all(void);

#define KDRIVER_REGISTER(driver_name, init_func) \
    kdriver_t driver_name##_driver __attribute__((section(".drivers"), aligned(4))) = { \
        .name = #driver_name, \
        .init = init_func, \
    };
    

#endif // AGAVE_KDRIVER_H