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

	printk("The AWS IoT sample started, version: %s\n", CONFIG_APP_VERSION);

	cJSON_Init();

	nrf_modem_lib_dfu_handler();

	err = aws_iot_init(NULL, aws_iot_event_handler);
	if (err) {
		printk("AWS IoT library could not be initialized, error: %d\n", err);
	}

	/** Subscribe to customizable non-shadow specific topics
	 *  to AWS IoT backend.
	 */
	err = app_topics_subscribe();
	if (err) {
		printk("Adding application specific topics failed, error: %d\n", err);
	}

	work_init();

	modem_configure();

	err = modem_info_init();
	if (err) {
		printk("Failed initializing modem info module, error: %d\n", err);
	}

	k_sem_take(&lte_connected, K_FOREVER);

	date_time_update_async(date_time_event_handler);
	k_work_schedule(&connect_work, K_NO_WAIT);
}

void work_init(void)
{
	k_work_init_delayable(&shadow_update_work, shadow_update_work_fn);
	k_work_init_delayable(&connect_work, connect_work_fn);
	k_work_init(&shadow_update_version_work, shadow_update_version_work_fn);
}

void connect_work_fn(struct k_work *work)
{
	int err;

	if (cloud_connected) {
		return;
	}

	err = aws_iot_connect(NULL);
	if (err) {
		printk("aws_iot_connect, error: %d\n", err);
	}

	printk("Next connection retry in %d seconds\n",
	       CONFIG_CONNECTION_RETRY_TIMEOUT_SECONDS);

	k_work_schedule(&connect_work,
			K_SECONDS(CONFIG_CONNECTION_RETRY_TIMEOUT_SECONDS));
}

void shadow_update_work_fn(struct k_work *work)
{
	int err;

	if (!cloud_connected) {
		return;
	}

	err = shadow_update(false);
	if (err) {
		printk("shadow_update, error: %d\n", err);
	}

	printk("Next data publication in %d seconds\n",
	       CONFIG_PUBLICATION_INTERVAL_SECONDS);

	k_work_schedule(&shadow_update_work,
			K_SECONDS(CONFIG_PUBLICATION_INTERVAL_SECONDS));
}

void shadow_update_version_work_fn(struct k_work *work)
{
	int err;

	err = shadow_update(true);
	if (err) {
		printk("shadow_update, error: %d\n", err);
	}
}