#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <date_time.h>

void date_time_event_handler(const struct date_time_evt *evt)
{
	switch (evt->type) {
		case DATE_TIME_OBTAINED_MODEM:
			printk("DATE_TIME_OBTAINED_MODEM\n");
			break;

		case DATE_TIME_OBTAINED_NTP:
			printk("DATE_TIME_OBTAINED_NTP\n");
			break;

		case DATE_TIME_OBTAINED_EXT:
			printk("DATE_TIME_OBTAINED_EXT\n");
			break;

		case DATE_TIME_NOT_OBTAINED:
			printk("DATE_TIME_NOT_OBTAINED\n");
			break;
			
		default:
			break;
	}
}