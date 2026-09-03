#include <syslog.h>
#include <cjson/cJSON.h>

#include "telemetry_json.h"

static int add_number_item(cJSON *root, const char *name, double value)
{
	cJSON *item;

	item = cJSON_CreateObject();
	if (item == NULL) {
		syslog(LOG_ERR, "Failed to create JSON item: %s", name);
		return -1;
	}

	cJSON_AddNumberToObject(item, "value", value);
	cJSON_AddItemToObject(root, name, item);

	return 0;
}

static int add_array_item(cJSON *root, const char *name, cJSON *array)
{
	cJSON *item;
 
	if (array == NULL) {
		syslog(LOG_ERR, "Array item is NULL: %s", name);
		return -1;
	}
 
	item = cJSON_CreateObject();
	if (item == NULL) {
		syslog(LOG_ERR, "Failed to create JSON item: %s", name);
		cJSON_Delete(array);
		return -1;
	}
 
	cJSON_AddItemToObject(item, "value", array);
	cJSON_AddItemToObject(root, name, item);
 
	return 0;
}

cJSON *build_telemetry_json(const struct system_info *system,
			    const struct network_info *interfaces,
			    int interface_count, double cpu_usage)
{
	cJSON *root;
	cJSON *interface_array;
	cJSON *ip_array;
	cJSON *netmask_array;
	cJSON *tx_array;
	cJSON *rx_array;
	int i;

	if (system == NULL || interface_count < 0) {
		syslog(LOG_ERR, "Invalid telemetry data");
		return NULL;
	}

	if (interface_count > 0 && interfaces == NULL) {
		syslog(LOG_ERR, "Network interfaces are NULL");
		return NULL;
	}

	root = cJSON_CreateObject();
	if (root == NULL) {
		syslog(LOG_ERR, "Failed to create JSON object");
		return NULL;
	}

	if (add_number_item(root, "total_ram",
			    system->total_ram / (1024.0 * 1024.0)) != 0) {
		cJSON_Delete(root);
		return NULL;
	}

	if (add_number_item(root, "free_ram",
			    system->free_ram / (1024.0 * 1024.0)) != 0) {
		cJSON_Delete(root);
		return NULL;
	}

	if (add_number_item(root, "system_uptime",
			    system->uptime) != 0) {
		cJSON_Delete(root);
		return NULL;
	}

	if (add_number_item(root, "cpu_usage", cpu_usage) != 0) {
		cJSON_Delete(root);
		return NULL;
	}

	interface_array = cJSON_CreateArray();
	ip_array = cJSON_CreateArray();
	netmask_array = cJSON_CreateArray();
	tx_array = cJSON_CreateArray();
	rx_array = cJSON_CreateArray();

	if (interface_array == NULL || ip_array == NULL ||
	    netmask_array == NULL || tx_array == NULL ||
	    rx_array == NULL) {
		syslog(LOG_ERR, "Failed to create network arrays");

		cJSON_Delete(interface_array);
		cJSON_Delete(ip_array);
		cJSON_Delete(netmask_array);
		cJSON_Delete(tx_array);
		cJSON_Delete(rx_array);
		cJSON_Delete(root);

		return NULL;
	}

	for (i = 0; i < interface_count; i++) {
		cJSON_AddItemToArray(
			interface_array,
			cJSON_CreateString(interfaces[i].name));

		cJSON_AddItemToArray(
			ip_array,
			cJSON_CreateString(interfaces[i].ip_address));

		cJSON_AddItemToArray(
			netmask_array,
			cJSON_CreateString(interfaces[i].netmask));

		/*
		 * Convert bytes to whole megabytes because Tuya
		 * accepts integer values for arrays.
		 */
		cJSON_AddItemToArray(
			tx_array,
			cJSON_CreateNumber(
				(int)(interfaces[i].tx_bytes /
				      (1024ULL * 1024ULL))));

		cJSON_AddItemToArray(
			rx_array,
			cJSON_CreateNumber(
				(int)(interfaces[i].rx_bytes /
				      (1024ULL * 1024ULL))));
	}

	if (add_array_item(root, "network_interface", interface_array) != 0 ||
	    add_array_item(root, "ip_address", ip_array) != 0 ||
	    add_array_item(root, "netmask", netmask_array) != 0 ||
	    add_array_item(root, "tx_mb", tx_array) != 0 ||
	    add_array_item(root, "rx_mb", rx_array) != 0) {
		cJSON_Delete(root);
		return NULL;
	}
	
	return root;
}