#ifndef TELEMETRY_JSON_H
#define TELEMETRY_JSON_H

#include <cjson/cJSON.h>

#include "system_info.h"
#include "network_info.h"

cJSON *build_telemetry_json(
    const struct system_info *system,
    const struct network_info *interfaces,
    int interface_count,
    double cpu_usage
);

#endif