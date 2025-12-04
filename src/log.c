#include "../include/common.h"
#include "../include/log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void log_event(const char *fmt, ...) {
    pthread_mutex_lock(&log_mutex);

    FILE *f = fopen("server.log", "a");
    if (!f) { pthread_mutex_unlock(&log_mutex); return; }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(f, "[%s] ", ts);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);

    fprintf(f, "\n");
    fclose(f);

    pthread_mutex_unlock(&log_mutex);
}
