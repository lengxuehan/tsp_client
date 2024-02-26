#ifndef TSP_CLIENT_TB_LOG_H
#define TSP_CLIENT_TB_LOG_H

#include <stdio.h>
#include <stdarg.h>

static void TB_LOG_INFO(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}

static void TB_LOG_ERROR(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}

#endif //TSP_CLIENT_TB_LOG_H
