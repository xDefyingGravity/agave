set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i686)

# Cross compilers
set(CMAKE_C_COMPILER /opt/homebrew/bin/i686-elf-gcc)
set(CMAKE_ASM_NASM_COMPILER /opt/homebrew/bin/nasm)
set(CMAKE_LINKER /opt/homebrew/bin/i686-elf-ld)
set(CMAKE_OBJCOPY /opt/homebrew/bin/i686-elf-objcopy)
set(CMAKE_RANLIB /opt/homebrew/bin/i686-elf-ranlib)

# C flags
set(CMAKE_C_FLAGS "-ffreestanding -O2 -Wall -Wextra -g")
set(CMAKE_EXE_LINKER_FLAGS "-Ttext 0x1000 -ffreestanding -nostdlib")
set(CMAKE_C_STANDARD 99)

# NASM flags
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_FLAGS "")