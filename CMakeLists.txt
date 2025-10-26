cmake_minimum_required(VERSION 4.0)
project(agave_kernel C ASM_NASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/toolchain.cmake)

# Sources
file(GLOB_RECURSE C_SOURCES "src/*.c")
file(GLOB_RECURSE NASM_SOURCES "src/boot/*.s" "src/asm/*.s")

# Compile NASM files
set(NASM_OBJECTS "")
foreach(asm_file ${NASM_SOURCES})
    get_filename_component(obj_file ${asm_file} NAME_WE)
    set(obj_path ${CMAKE_BINARY_DIR}/${obj_file}.o)
    add_custom_command(
        OUTPUT ${obj_path}
        COMMAND nasm -f elf32 ${asm_file} -o ${obj_path}
        DEPENDS ${asm_file}
    )
    list(APPEND NASM_OBJECTS ${obj_path})
endforeach()

add_executable(kernel.elf
    ${C_SOURCES}
    ${NASM_OBJECTS}
)

target_compile_definitions(kernel.elf PRIVATE
    KERNEL_VERSION="1.0.0"
    HEAP_SIZE=0x4000000
    TICK_FREQUENCY=1000
)

set_target_properties(kernel.elf PROPERTIES
    LINK_FLAGS "-T${CMAKE_SOURCE_DIR}/linker.ld -ffreestanding -nostdlib"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out
)

target_include_directories(kernel.elf PRIVATE include)

set_target_properties(kernel.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out
)

# ISO creation
add_custom_command(TARGET kernel.elf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/iso/boot
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel.elf> ${CMAKE_BINARY_DIR}/iso/boot/
    COMMAND i686-elf-grub-mkrescue -o ${CMAKE_BINARY_DIR}/out/kernel.iso ${CMAKE_BINARY_DIR}/iso
    COMMENT "Building ISO..."
)

add_custom_target(run
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target kernel.elf
    COMMAND qemu-system-i386 -m 512M -cdrom ${CMAKE_BINARY_DIR}/out/kernel.iso -boot d -monitor stdio
    COMMENT "Building and running the project..."
)