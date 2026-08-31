#include <stdio.h>
#include <syslog.h>
#include "cJSON.h"
#include "action.h"

void handle_action(const char *data)
{
    cJSON *root = NULL;
    cJSON *input_params = NULL;
    cJSON *text = NULL;

    if (data == NULL) {
        syslog(LOG_ERR, "Action data is NULL");
        return;
    }

    root = cJSON_Parse(data);

    if (root == NULL) {
        syslog(LOG_ERR, "Failed to parse action JSON");
        return;
    }

    input_params = cJSON_GetObjectItem(root, "inputParams");

    if (!cJSON_IsObject(input_params)) {
        syslog(LOG_ERR, "Action inputParams not found");
        cJSON_Delete(root);
        return;
    }

    text = cJSON_GetObjectItem(input_params, "text");

    if (!cJSON_IsString(text)) {
        syslog(LOG_ERR, "Action text parameter not found");
        cJSON_Delete(root);
        return;
    }

    FILE *file = fopen("/tmp/tuya_action.log", "a");

    if (file == NULL) {
        syslog(LOG_ERR, "Failed to open /tmp/tuya_action.log");
        cJSON_Delete(root);
        return;
    }

    fprintf(file, "%s\n", text->valuestring);

    fclose(file);

    syslog(LOG_INFO, "Action text saved to /tmp/tuya_action.log");

    cJSON_Delete(root);
}