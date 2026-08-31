#include <syslog.h>
#include <stddef.h>
#include <sys/sysinfo.h>

#include "system_info.h"

int collect_system_info(struct system_info *info)
{
    struct sysinfo system;

    if (info == NULL) {
        return -1;
    }

    if (sysinfo(&system) != 0) {
        syslog(LOG_ERR, "Failed to get system information");
        return -1;
    }

    info->total_ram = system.totalram * system.mem_unit;
    info->free_ram = system.freeram * system.mem_unit;
    info->uptime = system.uptime;

    return 0;
}