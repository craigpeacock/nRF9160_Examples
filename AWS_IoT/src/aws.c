#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <net/aws_iot.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_info.h>
#include <nrf_modem.h>
#include <cJSON.h>
#include <cJSON_os.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/dfu/mcuboot.h>
#include <date_time.h>
#include "json_support.h"
#include "aws.h"
#include "main.h"

#define PSM_ENABLED

#define AWS_IOT_PUB_CUSTOM_TOPIC "my-custom-topic/status"
#define AWS_IOT_SUB_CUSTOM_TOPIC "my-custom-topic/set"

#define MY_CUSTOM_TOPIC_1  "my-custom-topic/example"
#define MY_CUSTOM_TOPIC_2  "my-custom-topic/example2"

int app_topics_subscribe(void)
{
	int err;

	// By default, connection made to AWS IoT Broker is using a persistent 
	// session. This means the broker will store subscriptions. Any topic name 
	// modifications may not become active until the persistant connection 
	// session expiration time elapses.
	// https://docs.aws.amazon.com/iot/latest/developerguide/mqtt.html#mqtt-persistent-sessions

	// If modifing topic names, you may wish to temporary add the following
	// to your prj.conf to create a clean session on each connection.
	// CONFIG_MQTT_CLEAN_SESSION=y
		
	// If persistance sessions are enabled, on the nRF9160 console you should
	// see the following statement after aws_iot_connect() is called.
	//  AWS_IOT_EVT_CONNECTED
	// *Persistent session enabled

	static const struct mqtt_topic topic_list[] = {
		{
			.topic.utf8 = MY_CUSTOM_TOPIC_1,
			.topic.size = strlen(MY_CUSTOM_TOPIC_1),
			.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		},
		{
			.topic.utf8 = MY_CUSTOM_TOPIC_2,
			.topic.size = strlen(MY_CUSTOM_TOPIC_2),
			.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		}
	};

	err = aws_iot_application_topics_set(topic_list, ARRAY_SIZE(topic_list));
	if (err) {
		printk("aws_iot_subscription_topics_add, error: %d\n", err);
	}

	return err;
}

int shadow_update(bool version_number_include)
{
	int err;
	char *message;
	int64_t message_ts = 0;
	int16_t bat_voltage = 0;

	err = date_time_now(&message_ts);
	if (err) {
		printk("date_time_now, error: %d\n", err);
		return err;
	}

#if defined(CONFIG_NRF_MODEM_LIB)
	/* Request battery voltage data from the modem. */
	err = modem_info_short_get(MODEM_INFO_BATTERY, &bat_voltage);
	if (err != sizeof(bat_voltage)) {
		printk("modem_info_short_get, error: %d\n", err);
		return err;
	}
#endif

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *reported_obj = cJSON_CreateObject();

	if (root_obj == NULL || state_obj == NULL || reported_obj == NULL) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(reported_obj);
		err = -ENOMEM;
		return err;
	}

	if (version_number_include) {
		err = json_add_str(reported_obj, "app_version",
				    CONFIG_APP_VERSION);
	} else {
		err = 0;
	}

	err += json_add_number(reported_obj, "batv", bat_voltage);
	err += json_add_number(reported_obj, "ts", message_ts);
	err += json_add_obj(state_obj, "reported", reported_obj);
	err += json_add_obj(root_obj, "state", state_obj);

	if (err) {
		printk("json_add, error: %d\n", err);
		goto cleanup;
	}

	message = cJSON_Print(root_obj);
	if (message == NULL) {
		printk("cJSON_Print, error: returned NULL\n");
		err = -ENOMEM;
		goto cleanup;
	}

	struct aws_iot_data tx_data = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AWS_IOT_SHADOW_TOPIC_UPDATE,
		.ptr = message,
		.len = strlen(message)
	};

	printk("Publishing: %s to AWS IoT broker\n", message);

	err = aws_iot_send(&tx_data);
	if (err) {
		printk("aws_iot_send, error: %d\n", err);
	}

	cJSON_FreeString(message);

cleanup:

	cJSON_Delete(root_obj);

	return err;
}

int publish_custom_topic(void)
{
	int err;
	char *message;

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		cJSON_Delete(root_obj);
		err = -ENOMEM;
		return err;
	}

	err = json_add_number(root_obj, "Test", 1);
	
	if (err) {
		printk("json_add, error: %d\n", err);
		goto cleanup;
	}

	message = cJSON_Print(root_obj);
	if (message == NULL) {
		printk("cJSON_Print, error: returned NULL\n");
		err = -ENOMEM;
		goto cleanup;
	}

	struct aws_iot_data tx_data = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.str = AWS_IOT_PUB_CUSTOM_TOPIC,
		.topic.len = strlen(AWS_IOT_PUB_CUSTOM_TOPIC),
		.ptr = message,
		.len = strlen(message)
	};

	printk("Publishing: %s to AWS IoT broker\n", message);

	err = aws_iot_send(&tx_data);
	if (err) {
		printk("aws_iot_send, error: %d\n", err);
	}

	cJSON_FreeString(message);

cleanup:

	cJSON_Delete(root_obj);

	return err;
}

void print_received_data(const char *buf, const char *topic, size_t topic_len)
{
	char *str = NULL;
	cJSON *root_obj = NULL;

	root_obj = cJSON_Parse(buf);
	if (root_obj == NULL) {
		printk("cJSON Parse failure");
		return;
	}

	str = cJSON_Print(root_obj);
	if (str == NULL) {
		printk("Failed to print JSON object");
		goto clean_exit;
	}

	printf("Data received from AWS IoT console:\nTopic: %.*s\nMessage: %s\n",
		topic_len, topic, str);

	cJSON_FreeString(str);

clean_exit:
	cJSON_Delete(root_obj);
}

void process_received_data(const char *buf, const char *topic, size_t topic_len)
{
	// topic may not be null terminated, so need to use topic_len;
	if(strncmp(topic, AWS_IOT_SUB_CUSTOM_TOPIC, topic_len) == 0) {
		//printk("Found AWS_IOT_SUB_CUSTOM_TOPIC\n");
		process_custom_topic(buf, topic, topic_len);
	}
}

void process_custom_topic(const char *buf, const char *topic, size_t topic_len)
{
	// Parses a payload with the following format:
	// {
	//		"reset": 1
	// }

	cJSON *root_obj = NULL;
	int reset;

	root_obj = cJSON_Parse(buf);
	if (root_obj == NULL) {
		printk("cJSON Parse failure");
		return;
	}

	if (cJSON_HasObjectItem(root_obj, "reset")) {
		cJSON *item = NULL;
		item = cJSON_GetObjectItem(root_obj, "reset");
		reset = cJSON_GetNumberValue(item);
		printk("Reset = %d\n",reset);
	}

	cJSON_Delete(root_obj);
}

void aws_iot_event_handler(const struct aws_iot_evt *const evt)
{
	int err;

	switch (evt->type) {
		case AWS_IOT_EVT_CONNECTING:
			printk("AWS_IOT_EVT_CONNECTING\n");
			break;

		case AWS_IOT_EVT_CONNECTED:
			printk("AWS_IOT_EVT_CONNECTED\n");

			cloud_connected = true;
			/* This may fail if the work item is already being processed,
			* but in such case, the next time the work handler is executed,
			* it will exit after checking the above flag and the work will
			* not be scheduled again.
			*/
			(void)k_work_cancel_delayable(&connect_work);

			if (evt->data.persistent_session) {
				printk("Persistent session enabled\n");
			}

	#if defined(CONFIG_NRF_MODEM_LIB)
			/** Successfully connected to AWS IoT broker, mark image as
			 *  working to avoid reverting to the former image upon reboot.
			 */
			boot_write_img_confirmed();
	#endif

			/** Send version number to AWS IoT broker to verify that the
			 *  FOTA update worked.
			 */
			k_work_submit(&shadow_update_version_work);

			/** Start sequential shadow data updates.
			 */
			k_work_schedule(&shadow_update_work, K_SECONDS(CONFIG_PUBLICATION_INTERVAL_SECONDS));

#ifdef PSM_ENABLED
			err = lte_lc_psm_req(true);
			if (err) {
				printk("Requesting PSM failed, error: %d\n", err);
			}
#endif
			break;

		case AWS_IOT_EVT_DISCONNECTED:
			printk("AWS_IOT_EVT_DISCONNECTED\n");
			cloud_connected = false;
			/* This may fail if the work item is already being processed,
			* but in such case, the next time the work handler is executed,
			* it will exit after checking the above flag and the work will
			* not be scheduled again.
			*/
			(void)k_work_cancel_delayable(&shadow_update_work);
			k_work_schedule(&connect_work, K_NO_WAIT);
			break;

		case AWS_IOT_EVT_DATA_RECEIVED:
			printk("AWS_IOT_EVT_DATA_RECEIVED\n");
			print_received_data(evt->data.msg.ptr, evt->data.msg.topic.str,	evt->data.msg.topic.len);
			process_received_data(evt->data.msg.ptr, evt->data.msg.topic.str, evt->data.msg.topic.len);
			break;

		case AWS_IOT_EVT_FOTA_START:
			printk("AWS_IOT_EVT_FOTA_START\n");
			break;

		case AWS_IOT_EVT_FOTA_ERASE_PENDING:
			printk("AWS_IOT_EVT_FOTA_ERASE_PENDING\n");
			printk("Disconnect LTE link or reboot\n");
	#if defined(CONFIG_NRF_MODEM_LIB)
			err = lte_lc_offline();
			if (err) {
				printk("Error disconnecting from LTE\n");
			}
	#endif
			break;

		case AWS_IOT_EVT_FOTA_ERASE_DONE:
			printk("AWS_FOTA_EVT_ERASE_DONE\n");
			printk("Reconnecting the LTE link");
	#if defined(CONFIG_NRF_MODEM_LIB)
			err = lte_lc_connect();
			if (err) {
				printk("Error connecting to LTE\n");
			}
	#endif
			break;

		case AWS_IOT_EVT_FOTA_DONE:
			printk("AWS_IOT_EVT_FOTA_DONE\n");
			printk("FOTA done, rebooting device\n");
			aws_iot_disconnect();
			sys_reboot(0);
			break;

		case AWS_IOT_EVT_FOTA_DL_PROGRESS:
			printk("AWS_IOT_EVT_FOTA_DL_PROGRESS, (%d%%)",
				evt->data.fota_progress);

		case AWS_IOT_EVT_ERROR:
			printk("AWS_IOT_EVT_ERROR, %d\n", evt->data.err);
			break;

		case AWS_IOT_EVT_FOTA_ERROR:
			printk("AWS_IOT_EVT_FOTA_ERROR");
			break;

		default:
			printk("Unknown AWS IoT event type: %d\n", evt->type);
			break;
	}
}