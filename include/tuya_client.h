#ifndef TUYA_CLIENT_H
#define TUYA_CLIENT_H

int tuya_client_init(
    const char *device_id,
    const char *device_secret
);

int tuya_client_connect(void);

void tuya_client_loop(void);

void tuya_client_deinit(void);

int tuya_client_connected(void);

int tuya_client_report(const char *json);

#endif