#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

struct arguments {
    char *device_id;
    char *device_secret;
    char *product_id;
    bool daemon;
};

int parse_arguments(int argc, char **argv, struct arguments *arguments);

#endif