#include <syslog.h>
#include <stdlib.h>

#include <cjson/cJSON.h>

#include "telemetry.h"
#include "telemetry_json.h"
#include "system_info.h"
#include "network_info.h"
#include "cpu_info.h"
#include "tuya_client.h"

void telemetry_report(void)
{
	struct system_info system;
	struct network_info interfaces[MAX_INTERFACES];
	int interface_count;
	double cpu_usage;
	cJSON *root;
	char *json;
	int ret;

	if (collect_system_info(&system) != 0) {
		syslog(LOG_ERR, "Failed to collect system information");
		return;
	}

	interface_count = collect_network_info(interfaces, MAX_INTERFACES);

	if (interface_count < 0) {
		syslog(LOG_ERR, "Failed to collect network information");
		return;
	}

	if (get_cpu_usage(&cpu_usage) != 0) {
		syslog(LOG_ERR, "Failed to get CPU usage");
		return;
	}

	root = build_telemetry_json(&system,
				    interfaces,
				    interface_count,
				    cpu_usage);

	if (root == NULL)
		return;

	json = cJSON_PrintUnformatted(root);

	if (json == NULL) {
		syslog(LOG_ERR, "Failed to create JSON string");
		cJSON_Delete(root);
		return;
	}

	syslog(LOG_INFO, "Reporting data: %s", json);

	if (!tuya_client_connected()) {
		syslog(LOG_WARNING,
		       "Not connected to Tuya Cloud, skipping report");

		free(json);
		cJSON_Delete(root);
		return;
	}

	ret = tuya_client_report(json);

	if (ret < 0) {
		syslog(LOG_ERR,
		       "Failed to report data to Tuya Cloud: %d",
		       ret);
	} else {
		syslog(LOG_INFO,
		       "Data reported successfully, message ID: %d",
		       ret);
	}

	free(json);
	cJSON_Delete(root);
}