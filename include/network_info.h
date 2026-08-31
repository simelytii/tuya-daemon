#ifndef NETWORK_INFO_H
#define NETWORK_INFO_H

#include <netinet/in.h>

#define MAX_INTERFACES 16

struct network_info {
    char name[64];
    char ip_address[INET6_ADDRSTRLEN];
    char netmask[INET6_ADDRSTRLEN];
    unsigned long long tx_bytes;
    unsigned long long rx_bytes;
};

int collect_network_info(struct network_info *interfaces, int max_interfaces);

#endif
