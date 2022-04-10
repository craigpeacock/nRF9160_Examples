
#include <zephyr.h>
#include <sys/printk.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <date_time.h>

#include <posix/time.h>
#include <posix/sys/time.h>

void print_modem_info(enum modem_info info)
{
	int len;
	char buf[80];

	switch (info) {
		case MODEM_INFO_RSRP:
			printk("Signal Strength: ");
			break;
		case MODEM_INFO_IP_ADDRESS:
			printk("IP Addr: ");
			break;
		case MODEM_INFO_FW_VERSION:
			printk("Modem FW Ver: ");
			break;
		case MODEM_INFO_ICCID:
			printk("SIM ICCID: ");
			break;
		case MODEM_INFO_IMSI:
			printk("IMSI: ");
			break;
		case MODEM_INFO_IMEI:
			printk("IMEI: ");
			break;
		case MODEM_INFO_DATE_TIME:
			printk("Network Date/Time: ");
			break;
		case MODEM_INFO_APN:
			printk("APN: ");
			break;
		default:
			printk("Unsupported: ");
			break;
	}

	len = modem_info_string_get(info, buf, 80);
	if (len > 0) {
		printk("%s\n",buf);
    	//printk("%s (%d bytes)\n",buf,len);
	} else {
		printk("Error\n");
	}
}

void print_UTC_time(void)
{
	// The Nordic Date-Time library will set the zephyr operating system time, 
	// hence we can use standard POSIX functions to display time.
	struct timespec tp;
	struct tm timeinfo;
	char buf[50];

	if (clock_gettime(CLOCK_REALTIME, &tp) == 0) {
		gmtime_r(&tp, &timeinfo);
		printk("UTC Time: %s", asctime_r(&timeinfo, buf));
	} else {
		printk("Error obtaining time\n");
	}
}

static void date_time_event_handler(const struct date_time_evt *evt)
{
	switch (evt->type) {
		case DATE_TIME_OBTAINED_MODEM:
			printk("Date/Time obtained from modem, ");
			break;
		case DATE_TIME_OBTAINED_NTP:
			printk("Date/Time obtained from NTP, ");
			break;
		case DATE_TIME_OBTAINED_EXT:
			printk("Date/Time externally set (manual/GPS), ");
			break;
		case DATE_TIME_NOT_OBTAINED:
			printk("Date/Time unable to be obtained, ");
			break;
		default:
			break;
	}
	print_UTC_time();
}

void main(void)
{
	int err;

	printk("\nnRF9160 Basic Networking Example (%s)\n", CONFIG_BOARD);

	err = lte_lc_init();
	if (err) printk("MODEM: Failed initializing LTE Link controller, error: %d\n", err);

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_UICC);
	if (err) printk("MODEM: Failed enabling UICC power, error: %d\n", err);
	k_msleep(100);
	
	err = modem_info_init();
	if (err) printk("MODEM: Failed initializing modem info module, error: %d\n", err);

	print_modem_info(MODEM_INFO_FW_VERSION);
	print_modem_info(MODEM_INFO_IMEI);
	//print_modem_info(MODEM_INFO_ICCID);
	printk("\n");

	// We have enabled CONFIG_DATE_TIME_MODEM (use modem as date/time source) and
	// CONFIG_DATE_TIME_AUTO_UPDATE to trigger date-time update automatically when
	// LTE is connected. The event handler will show when we be obtain a time fix.
	date_time_register_handler(date_time_event_handler);


	printk("Waiting for network... ");
	err = lte_lc_init_and_connect();
	if (err) {
		printk("Failed to connect to the LTE network, err %d\n", err);
		return;
	}
	printk("OK\n");
	print_modem_info(MODEM_INFO_APN);
	print_modem_info(MODEM_INFO_IP_ADDRESS);
	//print_modem_info(MODEM_INFO_DATE_TIME);
	//print_modem_info(MODEM_INFO_RSRP);

	while (1) {

		/* 
		 * Do something here
		 */ 

		k_msleep(1000);
	}

}


