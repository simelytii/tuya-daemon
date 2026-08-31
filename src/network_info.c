#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <string.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdlib.h>

#include "network_info.h"

static unsigned long long read_interface_stat(
    const char *interface_name,
    const char *stat_name)
{
    char path[256];
    FILE *file;
    unsigned long long value = 0;

    snprintf(path,
             sizeof(path),
             "/sys/class/net/%s/statistics/%s",
             interface_name,
             stat_name);

    file = fopen(path, "r");

    if (file == NULL) {
        return 0;
    }

    if (fscanf(file, "%llu", &value) != 1) {
        value = 0;
    }

    fclose(file);

    return value;
}

int collect_network_info(struct network_info *interfaces, int max_interfaces)
{
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;
    int count = 0;

    if (interfaces == NULL || max_interfaces <= 0) {
        return -1;
    }

    if (getifaddrs(&ifaddr) == -1) {
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL && count < max_interfaces; ifa = ifa->ifa_next) {

        if (ifa->ifa_name == NULL || ifa->ifa_addr == NULL) {
            continue;
        }

        if (strcmp(ifa->ifa_name, "lo") == 0) {
            continue;
        }

        if (ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        strncpy(interfaces[count].name,
                ifa->ifa_name,
                sizeof(interfaces[count].name) - 1);

        interfaces[count].name[
            sizeof(interfaces[count].name) - 1
        ] = '\0';

        if (getnameinfo(ifa->ifa_addr,
                        sizeof(struct sockaddr_in),
                        interfaces[count].ip_address,
                        sizeof(interfaces[count].ip_address),
                        NULL,
                        0,
                        NI_NUMERICHOST) != 0) {

            interfaces[count].ip_address[0] = '\0';
        }

        if (ifa->ifa_netmask != NULL) {
            if (getnameinfo(ifa->ifa_netmask,
                            sizeof(struct sockaddr_in),
                            interfaces[count].netmask,
                            sizeof(interfaces[count].netmask),
                            NULL,
                            0,
                            NI_NUMERICHOST) != 0) {

                interfaces[count].netmask[0] = '\0';
            }
        } else {
            interfaces[count].netmask[0] = '\0';
        }
        
        interfaces[count].tx_bytes =
        read_interface_stat(interfaces[count].name, "tx_bytes");

        interfaces[count].rx_bytes =
        read_interface_stat(interfaces[count].name, "rx_bytes");
        count++;
    }

    freeifaddrs(ifaddr);

    return count;
}