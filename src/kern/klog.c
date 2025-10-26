#include <agave/klog.h>
#include <agave/kvid.h>
#include <agave/kutils.h>
#include <stdarg.h>

#define KLOG_INFO "info"
#define KLOG_WARN "warn"
#define KLOG_ERROR "error"

void klog_internal(const char* level, const char* fmt, va_list args) {
    kprintf("[%s] ", level);
    kvprintf(fmt, args);
}

void klog(klog_level_t level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    switch (level) {
        case KLOG_LEVEL_INFO:
            klog_internal(KLOG_INFO, fmt, args);
            break;
        case KLOG_LEVEL_WARN:
            klog_internal(KLOG_WARN, fmt, args);
            break;
        case KLOG_LEVEL_ERROR:
            klog_internal(KLOG_ERROR, fmt, args);
            break;
    }
    va_end(args);
}

void kinfo(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    klog_internal(KLOG_INFO, fmt, args);
    va_end(args);
}

void kwarn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    klog_internal(KLOG_WARN, fmt, args);
    va_end(args);
}

void kerror(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    klog_internal(KLOG_ERROR, fmt, args);
    va_end(args);
}