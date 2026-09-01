#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <string.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/socket.h>
#include <syslog.h>

#include "network_info.h"

static int read_interface_stat(const char *interface_name,
			       const char *stat_name,
			       unsigned long long *value)
{
	char path[256];
	FILE *file;

	if (value == NULL) {
		syslog(LOG_ERR, "Network statistic output pointer is NULL");
		return -1;
	}

	snprintf(path, sizeof(path),
		 "/sys/class/net/%s/statistics/%s",
		 interface_name, stat_name);

	file = fopen(path, "r");

	if (file == NULL) {
		syslog(LOG_ERR,
		       "Failed to open network statistics file: %s",
		       path);
		return -1;
	}

	if (fscanf(file, "%llu", value) != 1) {
		syslog(LOG_ERR,
		       "Failed to read network statistic: %s",
		       path);
		fclose(file);
		return -1;
	}

	fclose(file);

	return 0;
}

int collect_network_info(struct network_info *interfaces,
			 int max_interfaces)
{
	struct ifaddrs *ifaddr;
	struct ifaddrs *ifa;
	int count = 0;

	if (interfaces == NULL || max_interfaces <= 0) {
		syslog(LOG_ERR, "Invalid network information arguments");
		return -1;
	}

	if (getifaddrs(&ifaddr) == -1) {
		syslog(LOG_ERR, "Failed to get network interfaces");
		return -1;
	}

	for (ifa = ifaddr;
	     ifa != NULL && count < max_interfaces;
	     ifa = ifa->ifa_next) {
		if (ifa->ifa_name == NULL || ifa->ifa_addr == NULL)
			continue;

		if (strcmp(ifa->ifa_name, "lo") == 0)
			continue;

		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;

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

		if (read_interface_stat(interfaces[count].name,
					"tx_bytes",
					&interfaces[count].tx_bytes) != 0)
			interfaces[count].tx_bytes = 0;

		if (read_interface_stat(interfaces[count].name,
					"rx_bytes",
					&interfaces[count].rx_bytes) != 0)
			interfaces[count].rx_bytes = 0;

		count++;
	}

	freeifaddrs(ifaddr);

	return count;
}