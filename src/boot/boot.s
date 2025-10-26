; boot.s - minimal multiboot + 32-bit GDT

BITS 32

; -----------------------
; multiboot header
; -----------------------
section .multiboot header align=4
    dd 0x1BADB002              ; magic number
    dd 0x0                     ; flags
    dd -(0x1BADB002 + 0x0)     ; checksum

; -----------------------
; GDT
; -----------------------
section .data gdt align=8
gdt_start:
    dq 0x0000000000000000      ; null descriptor
    dq 0x00CF9A000000FFFF      ; code segment
    dq 0x00CF92000000FFFF      ; data segment
gdt_end:

gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; -----------------------
; text / entry point
; -----------------------
section .text boot
global _start
extern kmain

_start:
    cli
    xor ebp, ebp
    mov esp, 0x9F000

    ; load GDT
    lgdt [gdt_ptr]

    ; switch to protected mode
    mov eax, cr0
    or  eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode_entry

protected_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    call kmain

.halt_loop:
    hlt
    jmp .halt_loop