# nRF9160_Examples

Basic example code for the [Nordic nRF9160 LTE-M/NB-IoT modem](https://www.nordicsemi.com/Products/nRF9160) 

* [AWS_IoT](https://github.com/craigpeacock/nRF9160_Examples/tree/main/AWS_IoT) - Forked from the nRF SDK, this example adds extra features such as setting the AWS Client ID at runtime from the IMEI. Project is split up into individual files. 
* [Basic Networking](https://github.com/craigpeacock/nRF9160_Examples/tree/main/BasicNetworking) - Connects to the LTE Network, and downloads via HTTP approx 4KBytes of LorumIpsum.
* [GPS](https://github.com/craigpeacock/nRF9160_Examples/tree/main/GPS) - Obtains a GPS (Global Positioning System)/GNSS (Global Navigation Satellite System) Position Fix.
* [KeyMgmt](https://github.com/craigpeacock/nRF9160_Examples/tree/main/KeyMgmt) - Demo to see if we can store configuration settings (board configuration, hardware version, board serial number) in with credentials. Credentials are loaded over the AT host port during manufacturer and board settings could be loaded using the same method.
* [Time](https://github.com/craigpeacock/nRF9160_Examples/tree/main/Time) - Obtains time via LTE and examines UTC / LocalTime functions and issues.
* [fade_rgb_led](https://github.com/craigpeacock/nRF9160_Examples/tree/main/fade_rgb_led) - Using PWM, fades a RGB led in a cycle of Red, Green & Blue.
* [shtc3](https://github.com/craigpeacock/nRF9160_Examples/tree/main/shtc3) - Demonstrates using a Device Tree Overlay at an applications level to configure a Zephyr I2C Sensor.

# Development Board

* Firmware has been developed and tested on the [Lemon IoT - LTE nrf9160 board](http://lemon-iot.com)
