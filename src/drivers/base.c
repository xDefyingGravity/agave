#include <agave/kdriver.h>

#define MAX_DRIVERS 32
static kdriver_t* drivers[MAX_DRIVERS];
static size_t driver_count = 0;

void kdriver_register(kdriver_t* driver) {
    if (driver_count < MAX_DRIVERS) {
        drivers[driver_count++] = driver;
    }
}

extern kdriver_t __start_drivers[];
extern kdriver_t __stop_drivers[];

void kdriver_init_all(void) {
    for (kdriver_t* drv = __start_drivers; drv < __stop_drivers; drv++) {
        kdriver_register(drv);
    }

    for (size_t i = 0; i < driver_count; i++) {
        if (drivers[i]->init)
            drivers[i]->init();
    }
}