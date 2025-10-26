global irq1_handler
global irq0_handler

extern keyboard_handler
extern pit_callback

irq1_handler:
    pusha
    call keyboard_handler
    popa
    mov al, 0x20
    out 0x20, al
    iret

irq0_handler:
    pusha
    call pit_callback
    popa
    mov al, 0x20
    out 0x20, al
    iret