global isr_stub_table
extern exception_handler
extern irq0_handler
extern irq1_handler

; macro for exceptions with error code
%macro isr_err_stub 1
isr_stub_%1:
    cli
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    call exception_handler
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    iret
%endmacro

; macro for IRQs / exceptions without error code
%macro isr_no_err_stub 2
isr_stub_%1:
    cli
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    call %2
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    iret
%endmacro

; default handler for unused IRQs
default_irq_handler:
    mov al, 0x20
    out 0x20, al
    iret

; --- stubs ---
isr_no_err_stub 0, irq0_handler
isr_no_err_stub 1, irq1_handler
isr_no_err_stub 2, default_irq_handler
isr_no_err_stub 3, default_irq_handler
isr_no_err_stub 4, default_irq_handler
isr_no_err_stub 5, default_irq_handler
isr_no_err_stub 6, default_irq_handler
isr_no_err_stub 7, default_irq_handler
isr_err_stub    8
isr_no_err_stub 9, default_irq_handler
isr_err_stub   10
isr_no_err_stub 11, default_irq_handler
isr_err_stub   12
isr_err_stub   13
isr_err_stub   14
isr_no_err_stub 15, default_irq_handler
isr_no_err_stub 16, default_irq_handler
isr_no_err_stub 17, default_irq_handler
isr_err_stub   18
isr_no_err_stub 19, default_irq_handler
isr_no_err_stub 20, default_irq_handler
isr_no_err_stub 21, default_irq_handler
isr_no_err_stub 22, default_irq_handler
isr_no_err_stub 23, default_irq_handler
isr_no_err_stub 24, default_irq_handler
isr_no_err_stub 25, default_irq_handler
isr_no_err_stub 26, default_irq_handler
isr_no_err_stub 27, default_irq_handler
isr_no_err_stub 28, default_irq_handler
isr_no_err_stub 29, default_irq_handler
isr_no_err_stub 30, default_irq_handler
isr_no_err_stub 31, default_irq_handler

; --- ISR table ---
isr_stub_table:
%assign i 0
%rep 32
    dd isr_stub_%+i
%assign i i+1
%endrep