#include <agave/kutils.h>

char* kitoa(int value, char* buffer, int base) {
    char* ptr = buffer;
    char* ptr1;
    char tmp_char;
    int tmp_value;

    if (base < 2 || base > 16) {
        *buffer = '\0';
        return buffer;
    }

    if (value < 0 && base == 10) {
        *ptr++ = '-';
        value = -value;
    }

    ptr1 = ptr;

    do {
        tmp_value = value % base;
        *ptr++ = "0123456789abcdef"[tmp_value];
        value /= base;
    } while (value);

    *ptr-- = '\0';

    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return buffer;
}

char* kitoa_unsigned(unsigned int value, char* buffer, int base) {
    char* ptr = buffer;
    char* ptr1;
    char tmp_char;
    unsigned int tmp_value;

    if (base < 2 || base > 16) {
        *buffer = '\0';
        return buffer;
    }

    ptr1 = ptr;

    do {
        tmp_value = value % base;
        *ptr++ = "0123456789abcdef"[tmp_value];
        value /= base;
    } while (value);

    *ptr-- = '\0';

    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return buffer;
}
