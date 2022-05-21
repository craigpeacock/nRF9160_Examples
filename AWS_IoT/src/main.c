/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(CONFIG_NRF_MODEM_LIB)
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_info.h>
#include <nrf_modem.h>
#endif
#include <net/aws_iot.h>
#include <sys/reboot.h>
#include <date_time.h>
#include <dfu/mcuboot.h>
#include <cJSON.h>
#include <cJSON_os.h>

#include "datetime.h"
#include "lte.h"
#include "dfu.h"
#include "aws.h"
#include "json_support.h"
#include "main.h"

BUILD_ASSERT(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
		"This sample does not support LTE auto-init and connect");

struct k_work_delayable shadow_update_work;
struct k_work_delayable connect_work;
struct k_work shadow_update_version_work;

bool cloud_connected;

void main(void)
{
	int err;
	char modem_IMEI[16];
	struct aws_iot_config aws_config;

	printk("The AWS IoT sample started, version: %s\n", CONFIG_APP_VERSION);

	cJSON_Init();

	// This example supports Amazon Web Services firmware over-the-air (AWS FOTA)
	// Update. Check update status, if FW update has just occured.
	nrf_modem_lib_dfu_handler();

	// Initialise modem information module. Used to request battery voltage data 
	// from the modem
	err = modem_info_init();
	if (err) {
		printk("Failed initializing modem info module, error: %d\n", err);
	}

	// Obtain the modems' IMEI (International Mobile Equipment Identity). We use
	// this as the client ID (thing ID) when communicating with the AWS IoT Broker
	int len = modem_info_string_get(MODEM_INFO_IMEI, modem_IMEI, 16);
	if (len > 0) {
		printk("IMEI: %s\n",modem_IMEI);
	}

	// The AWS IOT Client ID should match the 'Thing Name' on the 
	// AWS IoT console. It can be set in the prj.conf using: 
	// CONFIG_AWS_IOT_CLIENT_ID_STATIC="<Thing Name>" 
	// or at run time using the aws_iot_config structure passed 
	// to aws_iot_connect(); If using this option, set 
	// CONFIG_AWS_IOT_CLIENT_ID_APP=y
	
	// Setting the Client ID at runtime allows the developer to 
	// easly deploy fleets of devices with the same firmware. 
	// In this case we read the modem's IMEI and use this as
	// the Client ID.

	aws_config.client_id = modem_IMEI;
	aws_config.client_id_len = sizeof(modem_IMEI);

	// Initialise Amazon Web Services IoT Module
	err = aws_iot_init(&aws_config, aws_iot_event_handler);
	if (err) {
		printk("AWS IoT library could not be initialized, error: %d\n", err);
	}

	// Subscribe to customizable non-shadow specific topics.
	// Topics will be subscribed to upon connection to AWS IoT broker.
	err = app_topics_subscribe();
	if (err) {
		printk("Adding application specific topics failed, error: %d\n", err);
	}

	// Initialise Workqueue Threads 
	// Thread connect_work is responsible for connecting and reconnecting to the 
	// AWS IoT broker.
	k_work_init_delayable(&connect_work, connect_work_fn);
	// Thread shadow_update_version_work only publishes a message (with version 
	// number) upon an initial connection. 
	k_work_init(&shadow_update_version_work, shadow_update_version_work_fn);
	// Thread shadow_update_work continues to periodically publish a message 
	// (without version number)
	k_work_init_delayable(&shadow_update_work, shadow_update_work_fn);
		
	// Initialise LTE modem and LTE handler callback
	modem_configure();

	// Wait for LTE modem to connect 
	k_sem_take(&lte_connected, K_FOREVER);

	// Update date & time
	date_time_update_async(date_time_event_handler);

	// Initiate connection to AWS IoT MQTT 
	k_work_schedule(&connect_work, K_NO_WAIT);
}

void connect_work_fn(struct k_work *work)
{
	int err;

	if (cloud_connected) {
		return;
	}

	// Connect to AWS IoT broker (MQTT)
	err = aws_iot_connect(NULL);
	if (err) {
		printk("aws_iot_connect, error: %d\n", err);
	}

	printk("Next connection retry in %d seconds\n", CONFIG_CONNECTION_RETRY_TIMEOUT_SECONDS);

	k_work_schedule(&connect_work, K_SECONDS(CONFIG_CONNECTION_RETRY_TIMEOUT_SECONDS));
}

void shadow_update_work_fn(struct k_work *work)
{
	int err;

	if (!cloud_connected) {
		return;
	}

	// Update shadow with-out version number (Type 1)
	err = shadow_update(false);
	if (err) {
		printk("shadow_update, error: %d\n", err);
	}

	printk("Next data publication in %d seconds\n", CONFIG_PUBLICATION_INTERVAL_SECONDS);

	k_work_schedule(&shadow_update_work, K_SECONDS(CONFIG_PUBLICATION_INTERVAL_SECONDS));
}

void shadow_update_version_work_fn(struct k_work *work)
{
	int err;

	// Update shadow with version number (Type 2)
	err = shadow_update(true);
	if (err) {
		printk("shadow_update, error: %d\n", err);
	}
}