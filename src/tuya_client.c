#include <syslog.h>

#include "tuya_client.h"
#include "tuyalink_core.h"
#include "tuya_cacert.h"
#include "action.h"

tuya_mqtt_context_t client_instance;

static void on_connected(tuya_mqtt_context_t *context,
			 void *user_data)
{
	(void)context;
	(void)user_data;

	syslog(LOG_INFO, "Connected to Tuya Cloud");
}

static void on_disconnect(tuya_mqtt_context_t *context,
			  void *user_data)
{
	(void)context;
	(void)user_data;

	syslog(LOG_INFO, "Disconnected from Tuya Cloud");
}

static void on_tuya_message(tuya_mqtt_context_t *context,
			    void *user_data,
			    const tuyalink_message_t *msg)
{
	(void)context;
	(void)user_data;

	if (msg == NULL) {
		syslog(LOG_ERR, "Received NULL message");
		return;
	}

	if (msg->type != THING_TYPE_ACTION_EXECUTE)
		return;

	if (msg->data_string == NULL) {
		syslog(LOG_ERR, "Action data is NULL");
		return;
	}

	handle_action(msg->data_string);
}

int tuya_client_init(const char *device_id,
		     const char *device_secret)
{
	return tuya_mqtt_init(
		&client_instance,
		&(const tuya_mqtt_config_t) {
			.host = "m1.tuyacn.com",
			.port = 8883,
			.cacert = (const uint8_t *)tuya_cacert_pem,
			.cacert_len = sizeof(tuya_cacert_pem),
			.device_id = device_id,
			.device_secret = device_secret,
			.keepalive = 100,
			.timeout_ms = 2000,
			.on_connected = on_connected,
			.on_disconnect = on_disconnect,
			.on_messages = on_tuya_message,
		}
	);
}

int tuya_client_connect(void)
{
	return tuya_mqtt_connect(&client_instance);
}

void tuya_client_loop(void)
{
	tuya_mqtt_loop(&client_instance);
}

void tuya_client_deinit(void)
{
	tuya_mqtt_deinit(&client_instance);
}

int tuya_client_connected(void)
{
	return tuya_mqtt_connected(&client_instance);
}

int tuya_client_report(const char *json)
{
	return tuyalink_thing_property_report(
		&client_instance,
		NULL,
		json
	);
}