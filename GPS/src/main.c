#include <zephyr.h>
#include <sys/printk.h>
#include <modem/lte_lc.h>
#include <nrf_modem_gnss.h>

static K_SEM_DEFINE(pvt_data_sem, 0, 1);

static struct nrf_modem_gnss_pvt_data_frame pvt_data;

static void gnss_event_handler(int event)
{
	switch (event) {
		
		case NRF_MODEM_GNSS_EVT_PVT:
			//printk("NRF_MODEM_GNSS_EVT_PVT:\n");
			if (nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT) != 0) {
				printk("Error reading PVT data\n");
			} else {
				k_sem_give(&pvt_data_sem);
			}
			break;

		case NRF_MODEM_GNSS_EVT_FIX:
			//printk("NRF_MODEM_GNSS_EVT_FIX:\n");
			break;

		case NRF_MODEM_GNSS_EVT_NMEA:
			//printk("NRF_MODEM_GNSS_EVT_NMEA:\n");
			break;

		case NRF_MODEM_GNSS_EVT_AGPS_REQ:
			printk("NRF_MODEM_GNSS_EVT_AGPS_REQ:\n");
			break;

		default:
			printk("Unknown GNSS Event %d\n",event);
			break;
	}
}

void main(void)
{
	int err;

	printk("\nnRF9160 GNSS Example (%s)\n", CONFIG_BOARD);

	if (err = lte_lc_init()) {
		printk("Failed initializing LTE Link controller, error: %d\n", err);
	}

	if (err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS)) {
		printk("Failed enabling GNSS Functional Mode, error: %d\n", err);
	}

	if (err = nrf_modem_gnss_event_handler_set(gnss_event_handler)) {
		printk("Failed to set GNSS event handler, error: %d\n", err);
	}

	// Set to 0 for Single fix mode. GNSS will switch off after valid
	// fix or when fix_retry_set has been reached.
	// Set to 1 for Continuous navigation. GNSS receiver to produce 
	// PVT @ 1Hz
	nrf_modem_gnss_fix_interval_set(1);
	// Number of times to try for a fix
	nrf_modem_gnss_fix_retry_set(0);

	// By default, all NMEA strings are disabled.
#ifdef NMEA
	/* Enable all supported NMEA messages. */
	uint16_t nmea_mask = 	NRF_MODEM_GNSS_NMEA_RMC_MASK |
				     		NRF_MODEM_GNSS_NMEA_GGA_MASK |
			     			NRF_MODEM_GNSS_NMEA_GLL_MASK |
			     			NRF_MODEM_GNSS_NMEA_GSA_MASK |
			     			NRF_MODEM_GNSS_NMEA_GSV_MASK;

	if (nrf_modem_gnss_nmea_mask_set(nmea_mask) != 0) {
		printk("Failed to set GNSS NMEA mask\n");
	}
#endif

	// Enable low accuracy mode
	if (err = nrf_modem_gnss_use_case_set(NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY)) {
		printk("Failed to set low accuracy mode, error %d\n",err);
	}

	if (err = nrf_modem_gnss_start() != 0) {
		printk("Failed to start GNSS, error %d\n",err);
	}

	while (1) {

	    if (k_sem_take(&pvt_data_sem, K_NO_WAIT) != 0) {
    	    //printk("Input data not available!");
    	} else {

			if (pvt_data.flags && NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
				printk("Latitude: %.06f Longitude: %.06f\n", pvt_data.latitude, pvt_data.longitude);
			} else {
				int found = 0;
				//int fix = 0;
				for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; ++i) {
					if (pvt_data.sv[i].sv > 0) {
						found++;
						//Print satelite number:
						//printf("SV Number %d ", pvt_data.sv[i].sv);

						//Flag is only set once we have a fix, and at that
						//stage, this routine is no longer called.						
						//if (pvt_data.sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX) fix++;
					}
				}
				printk("Searching: Found %d SV\n",found);
			}
    	}

	}

	if (err = nrf_modem_gnss_stop() != 0) {
		printk("Failed to stop GNSS, error %d\n",err);
	}
}