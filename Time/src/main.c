
#include <zephyr.h>
#include <sys/printk.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <date_time.h>

#include <posix/time.h>
#include <posix/sys/time.h>

// Global variables used by the Newlib C Library Time Functions
extern long _timezone;
extern int _daylight;
extern char *_tzname[2];

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
	char buf[50];

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

	// Wait for device to obtain an initial valid date time
	while (!date_time_is_valid());

	// The Date-Time library obtains the LTE network time using the AT command +CCLK?
	// where the response is +CCLK: "yy/MM/dd,hh:mm:ss±zz". However it appears the library
	// ignores the timezone information (zz). Time zone indicates the difference, expressed 
	// in quarters of an hour, between the local time and UTC (value range from −48 to +48).
	//
	//		rc = nrf_modem_at_scanf("AT+CCLK?",
	//			"+CCLK: \"%u/%u/%u,%u:%u:%u",
	//			&date_time.tm_year,
	//			&date_time.tm_mon,
	//			&date_time.tm_mday,
	//			&date_time.tm_hour,
	//			&date_time.tm_min,
	//			&date_time.tm_sec
	//		);
	//
	// Ref: https://github.com/nrfconnect/sdk-nrf/blob/main/lib/date_time/date_time_modem.c

	// We can fetch our own copy using:
	err = nrf_modem_at_cmd(buf, 50, "AT+CCLK?");
	if (err == 0) printf("Response %s\n",buf);

	// Zephyr RTOS keeps track of the UTC time only, it has no concept of local time. 
	// Localtime support comes from the newlib c libraries.

	// Exampe of how to Manually Set TimeZone. This could conceivably happen from
	// parsing the +CCLK? response.
	putenv("TZ=GMT-9:30");
	tzset();

	// The following global variables are used by the Newlib C Library Time Functions
	// Setting the timezone offset appears to have no effect to localtime_r.
	// From the source, it appears localtime_r uses tzinfo structure instead, 
	// populated by calls to tzset();
	// https://sourceware.org/git/?p=newlib-cygwin.git;a=tree;f=newlib/libc/time
	printf("timezone = %ld\n",_timezone);
	printf("daylight %d\n",_daylight);
	printf("tz %s\n",*_tzname);
	
	while(1) {

		// Use standard timezone aware POSIX Functions from the newlib C Library,
		// gmtime_r	- function converts the calendar time into broken-down time 
		//            representation, expressed in Coordinated Universal Time (UTC).
		// localtime_r - function converts the calendar time into broken-down time 
		//               representation, expressed relative to the user's specified 
		//               timezone
		// This requires the timezone to be configured using tzset() as per above.  

		struct timespec tp;
		struct tm timeinfo;

		if (clock_gettime(CLOCK_REALTIME, &tp) == 0) {
			gmtime_r(&tp, &timeinfo);
			printk("UTC Time:   %s", asctime_r(&timeinfo, (char *)buf));

			localtime_r(&tp, &timeinfo);
			printk("Local Time: %s", asctime_r(&timeinfo, (char *)buf));
		}

		// Alternatively, we can manually hack the offset. This may actually be easier.
		if (clock_gettime(CLOCK_REALTIME, &tp) == 0) {
			gmtime_r(&tp, &timeinfo);
			printk("UTC Time:   %s", asctime_r(&timeinfo, buf));
			
			// The timezone information from +CCLK is in quarters of an hour (900 
			// seconds). We simply add or subtract the offset from the total number
			// of seconds since epoch. 
			tp.tv_sec += (38 * 900);

			gmtime_r(&tp, &timeinfo);
			printk("Local Time: %s", asctime_r(&timeinfo, buf));

			// If you need to schedule a task at local time and need to compare with
			// broken-down time values, you can use, for example:
			// timeinfo.tm_hour
			// timeinfo.tm_sec;

		} else {
			printk("Error obtaining time\n");
		}

		// Print timestamp using Nordic Date-Time Library Functions 
		// Timestamp is number of milli-seconds since epoch, January 1st 00:00:00 UTC 1970 
		// https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/date_time.html
		//
		// A review of the source code shows date_time_now calls clock_gettime();
		// https://github.com/nrfconnect/sdk-nrf/tree/main/lib/date_time
		
		int64_t ts;

		err = date_time_now(&ts);
		printk("Timestamp: ");
		if (err == -ENODATA) {
			printk("No valid date/time\n");
		} else if (err) {
			printk("Error %d obtaining date/time\n",err);
		} else { 
			printk("%llu\n", ts);
		}

		printf("\n");
		k_msleep(1000);
	}
}


