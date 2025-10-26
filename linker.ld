OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
    . = 0x100000;
    .text : { *(.multiboot) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .drivers : ALIGN(4) { __start_drivers = .; *(.drivers); __stop_drivers = .; }
    .bss : { *(.bss*) }
    _end = .;
}