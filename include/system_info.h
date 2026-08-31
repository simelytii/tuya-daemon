#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

struct system_info {
    unsigned long total_ram;
    unsigned long free_ram;
    long uptime;
};

int collect_system_info(struct system_info *info);

#endif