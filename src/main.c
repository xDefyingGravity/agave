#include <agave/ktimer.h>
#include <agave/kcore.h>
#include <agave/pit.h>
#include <agave/idt.h>
#include <agave/pic.h>
#include <agave/terminal.h>
#include <agave/drivers/keyboard.h>
#include <agave/kdriver.h>
#include <agave/kvid.h>
#include <agave/kutils.h>
#include <agave/kmem.h>

void kmain() {
    pic_remap();
    idt_init();

    ktimer_initialize(TICK_FREQUENCY);
    kdriver_init_all();
    
    flush_keyboard_buffer();

    kcore_initialize();
    terminal_initialize(true);

    kenable_interrupts();

    inf_idle();
} 