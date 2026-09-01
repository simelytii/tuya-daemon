#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "daemon.h"
#include "system_info.h"
#include "tuyalink_core.h"
#include "tuya_cacert.h"
#include "action.h"
#include "network_info.h"
#include "cpu_info.h"


struct arguments {
    char *device_id;
    char *device_secret;
    char *product_id;
    bool daemon;
};

static struct argp_option options[] = {
    {"device-id", 'd', "DEVICE_ID", 0, "Tuya device ID", 0},
    {"device-secret", 's', "DEVICE_SECRET", 0, "Tuya device secret", 0},
    {"product-id", 'p', "PRODUCT_ID", 0, "Tuya product ID", 0},
    {"daemon", 'D', 0, 0, "Run as daemon", 0},
    {0}
};

static tuya_mqtt_context_t client_instance;

static void on_connected(tuya_mqtt_context_t *context, void *user_data)
{
    (void)context;
    (void)user_data;

    syslog(LOG_INFO, "Connected to Tuya Cloud");
}

static void on_disconnect(tuya_mqtt_context_t *context, void *user_data)
{
    (void)context;
    (void)user_data;

    syslog(LOG_INFO, "Disconnected from Tuya Cloud");
}

static void on_tuya_message(
    tuya_mqtt_context_t *context,
    void *user_data,
    const tuyalink_message_t *msg)
{
    (void)context;
    (void)user_data;

    if (msg == NULL) {
        syslog(LOG_ERR, "Received NULL message");
        return;
    }

    if (msg->type != THING_TYPE_ACTION_EXECUTE) {
        return;
    }

    if (msg->data_string == NULL) {
        syslog(LOG_ERR, "Action data is NULL");
        return;
    }

    handle_action(msg->data_string);
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key) {
        case 'd':
            arguments->device_id = arg;
            break;

        case 's':
            arguments->device_secret = arg;
            break;

        case 'p':
            arguments->product_id = arg;
            break;

        case 'D':
            arguments->daemon = true;
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = {
    options,
    parse_opt,
    0,
    "Tuya IoT monitoring daemon",
    0,
    0,
    0
};

static void report_system_info(void)
{
    struct system_info system;
    struct network_info interfaces[MAX_INTERFACES];
    int interface_count;
    double cpu_usage;
    cJSON *root;
    cJSON *item;
    char *json;
    int i;

    if (collect_system_info(&system) != 0) {
        syslog(LOG_ERR, "Failed to collect system information");
        return;
    }

    interface_count = collect_network_info(
        interfaces,
        MAX_INTERFACES
    );

    if (interface_count < 0) {
        syslog(LOG_ERR, "Failed to collect network information");
        return;
    }

    if (get_cpu_usage(&cpu_usage) != 0) {
        syslog(LOG_ERR, "Failed to get CPU usage");
        return;
    }

    root = cJSON_CreateObject();

    if (root == NULL) {
        syslog(LOG_ERR, "Failed to create JSON object");
        return;
    }

    item = cJSON_CreateObject();
    cJSON_AddNumberToObject(item, "value", system.total_ram / (1024.0 * 1024.0));
    cJSON_AddItemToObject(root, "total_ram", item);

    item = cJSON_CreateObject();
    cJSON_AddNumberToObject(item, "value", system.free_ram / (1024.0 * 1024.0));
    cJSON_AddItemToObject(root, "free_ram", item);

    item = cJSON_CreateObject();
    cJSON_AddNumberToObject(item, "value", system.uptime);
    cJSON_AddItemToObject(root, "system_uptime", item);

    item = cJSON_CreateObject();
    cJSON_AddNumberToObject(item, "value", cpu_usage);
    cJSON_AddItemToObject(root, "cpu_usage", item);

    cJSON *interface_array;
    cJSON *ip_array;
    cJSON *netmask_array;
    cJSON *tx_array;
    cJSON *rx_array;

    interface_array = cJSON_CreateArray();
    ip_array = cJSON_CreateArray();
    netmask_array = cJSON_CreateArray();
    tx_array = cJSON_CreateArray();
    rx_array = cJSON_CreateArray();

    if (interface_array == NULL ||
        ip_array == NULL ||
        netmask_array == NULL ||
        tx_array == NULL ||
        rx_array == NULL) {

        syslog(LOG_ERR, "Failed to create network arrays");

        cJSON_Delete(interface_array);
        cJSON_Delete(ip_array);
        cJSON_Delete(netmask_array);
        cJSON_Delete(tx_array);
        cJSON_Delete(rx_array);
        cJSON_Delete(root);

        return;
    }

    for (i = 0; i < interface_count; i++) {

        cJSON_AddItemToArray(
            interface_array,
            cJSON_CreateString(interfaces[i].name)
        );

        cJSON_AddItemToArray(
            ip_array,
            cJSON_CreateString(interfaces[i].ip_address)
        );

        cJSON_AddItemToArray(
            netmask_array,
            cJSON_CreateString(interfaces[i].netmask)
        );

        cJSON_AddItemToArray(
            tx_array,
            cJSON_CreateNumber(
            (int)(interfaces[i].tx_bytes / (1024ULL * 1024ULL))
        )
        );

        cJSON_AddItemToArray(
            rx_array,
            cJSON_CreateNumber(
            (int)(interfaces[i].rx_bytes / (1024ULL * 1024ULL))
        )
        );
    }

    cJSON_AddItemToObject(root, "network_interface", interface_array);
    cJSON_AddItemToObject(root, "ip_address", ip_array);
    cJSON_AddItemToObject(root, "netmask", netmask_array);
    cJSON_AddItemToObject(root, "tx_mb", tx_array);
    cJSON_AddItemToObject(root, "rx_mb", rx_array);

    json = cJSON_PrintUnformatted(root);

    if (json == NULL) {
        syslog(LOG_ERR, "Failed to create JSON string");
        cJSON_Delete(root);
        return;
    }

    syslog(LOG_INFO, "Reporting data: %s", json);

    if (!tuya_mqtt_connected(&client_instance)) {
        syslog(LOG_WARNING, "Not connected to Tuya Cloud, skipping report");
        free(json);
        cJSON_Delete(root);
        return;
    }

    int ret;

    ret = tuyalink_thing_property_report(
        &client_instance,
        NULL,
        json
    );

    if (ret < 0) {
        syslog(LOG_ERR, "Failed to report data to Tuya Cloud: %d", ret);
    } else {
        syslog(LOG_INFO, "Data reported successfully, message ID: %d", ret);
    }

    free(json);
    cJSON_Delete(root);
}

int main(int argc, char **argv)
{
    struct arguments arguments = {0};

    openlog("tuya-daemon", LOG_PID | LOG_CONS, LOG_USER);

    error_t error;

    error = argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (error != 0) {
    syslog(LOG_ERR, "Failed to parse arguments");
    closelog();
    return EXIT_FAILURE;
    }

    if (arguments.device_id == NULL ||
        arguments.device_secret == NULL ||
        arguments.product_id == NULL) {

        syslog(LOG_ERR, "Missing required arguments");
        closelog();
        return EXIT_FAILURE;
    }

    if (arguments.daemon) {
        if (daemonize() != 0) {
            syslog(LOG_ERR, "Failed to daemonize");
            closelog();
            return EXIT_FAILURE;
        }

        syslog(LOG_INFO, "Running as daemon");
    }

        int ret;

    ret = tuya_mqtt_init(&client_instance, &(const tuya_mqtt_config_t) {
        .host = "m1.tuyacn.com",
        .port = 8883,
        .cacert = (const uint8_t *)tuya_cacert_pem,
        .cacert_len = sizeof(tuya_cacert_pem),
        .device_id = arguments.device_id,
        .device_secret = arguments.device_secret,
        .keepalive = 100,
        .timeout_ms = 2000,
        .on_connected = on_connected,
        .on_disconnect = on_disconnect,
        .on_messages = on_tuya_message,
    });

    if (ret != 0) {
        syslog(LOG_ERR, "Failed to initialize Tuya MQTT: %d", ret);
        closelog();
        return EXIT_FAILURE;
    }

    ret = tuya_mqtt_connect(&client_instance);

    if (ret != 0) {
        syslog(LOG_ERR, "Failed to connect to Tuya Cloud: %d", ret);
        tuya_mqtt_deinit(&client_instance);
        closelog();
        return EXIT_FAILURE;
    }

    while (1) {
        tuya_mqtt_loop(&client_instance);

        report_system_info();

        sleep(10);
    }

    sleep(10);

    closelog();
    return EXIT_SUCCESS;
}