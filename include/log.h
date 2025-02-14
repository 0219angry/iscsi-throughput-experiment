// log.h
#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <time.h>

#define LOG(fmt, ...) do { \
    time_t t = time(NULL); \
    struct tm *tm_info = localtime(&t); \
    char time_buf[20]; \
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info); \
    fprintf(stdout, "%s | %-15s:%-4d | %-20s | " fmt "\n", \
        time_buf, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
} while (0)

#endif // LOG_H
