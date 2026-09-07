#include <argp.h>

#include "cli.h"

const char *argp_program_version = "tuya-daemon 1.0";
const char *argp_program_bug_address = "<vak.simelyte@gmail.com>";

static struct argp_option options[] = {
    {"device-id", 'd', "DEVICE_ID", 0, "Tuya device ID", 0},
    {"device-secret", 's', "DEVICE_SECRET", 0, "Tuya device secret", 0},
    {"product-id", 'p', "PRODUCT_ID", 0, "Tuya product ID", 0},
    {"daemon", 'D', 0, 0, "Run as daemon", 0},
    {0}
};

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
    "Tuya IoT monitoring daemon that reports memory information "
    "system uptime, network interfaces and CPU usage to TuYa IoT "
    "Platform.\v"
    "Example:\n"
    "./tuya-daemon -d <device_id> -s <device_secret> -p <product_id> -D\n"
    "\n"
    "Device ID, secret and product ID are found on the Tuya IoT "
    "platform under project's device page.",
    0,
    0,
    0
};

int parse_arguments(int argc, char **argv, struct arguments *arguments)
{
    return argp_parse(&argp, argc, argv, 0, 0, arguments);
}