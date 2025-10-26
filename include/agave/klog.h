#ifndef AGAVE_KLOG_H
#define AGAVE_KLOG_H

#include <agave/kvid.h>

typedef enum klog_level {
    KLOG_LEVEL_INFO,
    KLOG_LEVEL_WARN,
    KLOG_LEVEL_ERROR,
} klog_level_t;

void klog(klog_level_t level, const char* format, ...);

void kinfo(const char* format, ...);
void kwarn(const char* format, ...);
void kerror(const char* format, ...);

#endif //AGAVE_KLOG_H