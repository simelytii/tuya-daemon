#include <stdio.h>

#include "cpu_info.h"

static int read_cpu_times(unsigned long long *idle,
                          unsigned long long *total)
{
    FILE *file;
    char cpu[5];
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle_time;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;

    file = fopen("/proc/stat", "r");

    if (file == NULL) {
        return -1;
    }

    if (fscanf(file,
               "%4s %llu %llu %llu %llu %llu %llu %llu %llu",
               cpu,
               &user,
               &nice,
               &system,
               &idle_time,
               &iowait,
               &irq,
               &softirq,
               &steal) != 9) {

        fclose(file);
        return -1;
    }

    fclose(file);

    *idle = idle_time + iowait;

    *total = user + nice + system + idle_time +
             iowait + irq + softirq + steal;

    return 0;
}

int get_cpu_usage(double *usage)
{
    static unsigned long long previous_idle = 0;
    static unsigned long long previous_total = 0;

    unsigned long long current_idle;
    unsigned long long current_total;
    unsigned long long idle_delta;
    unsigned long long total_delta;

    if (usage == NULL) {
        return -1;
    }

    if (read_cpu_times(&current_idle, &current_total) != 0) {
        return -1;
    }

    /*
     * First call only initializes the previous values.
     */
    if (previous_total == 0) {
        previous_idle = current_idle;
        previous_total = current_total;
        *usage = 0.0;
        return 0;
    }

    idle_delta = current_idle - previous_idle;
    total_delta = current_total - previous_total;

    if (total_delta == 0) {
        *usage = 0.0;
    } else {
        *usage = 100.0 *
                 (1.0 - ((double)idle_delta / total_delta));
    }

    previous_idle = current_idle;
    previous_total = current_total;

    return 0;
}