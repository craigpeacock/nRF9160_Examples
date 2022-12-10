/*
 * nRF9160 Sensirion SHTCx Example
 *
 * This example uses Zephyr Sensor Framework and DeviceTree overlays (at a 
 * project level) to obtain the temperature and humidity from a SHTC3 sensor.
 * 
 * API Reference:
 * https://docs.zephyrproject.org/latest/hardware/peripherals/sensor.html
 * https://docs.zephyrproject.org/3.0.0/reference/devicetree/bindings/sensor/sensirion%2Cshtcx.html
 * 
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

void main(void)
{
 	// Get a device structure from a devicetree node with compatible
 	// "sensirion,shtcx". 
	const struct device *dev = DEVICE_DT_GET_ANY(sensirion_shtcx);

	if (dev == NULL) {
		printk("\nError: No devicetree node found for Sensirion SHTCx.\n");
		return;
	}

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return;
	}

	printk("Found device %s. Reading sensor data\n", dev->name);

	while (true) {
		struct sensor_value temp, hum;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum);
		printk("%.2f degC, %0.2f%% RH\n", sensor_value_to_double(&temp), sensor_value_to_double(&hum));

		k_sleep(K_MSEC(1000));
	}
}
