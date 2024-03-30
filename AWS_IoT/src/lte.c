#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(CONFIG_NRF_MODEM_LIB)
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_info.h>
#include <nrf_modem.h>
#endif
#include "lte.h"

K_SEM_DEFINE(lte_connected, 0, 1);

void modem_configure(void)
{
	int err;

	err = lte_lc_connect_async(lte_handler);
	if (err) {
		printk("Modem could not be configured, err %d\n", err);
		return;
	}
}

void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
		case LTE_LC_EVT_NW_REG_STATUS:
			if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
				(evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
				break;
			}

			printk("Network registration status: %s\n",
				evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
				"Connected - home network" : "Connected - roaming");

			k_sem_give(&lte_connected);
			break;

		case LTE_LC_EVT_PSM_UPDATE:
			printk("PSM parameter update: TAU: %d, Active time: %d\n",
				evt->psm_cfg.tau, evt->psm_cfg.active_time);
			break;
			
		case LTE_LC_EVT_EDRX_UPDATE: {
			char log_buf[60];
			ssize_t len;

			len = snprintf(log_buf, sizeof(log_buf),
					"eDRX parameter update: eDRX: %f, PTW: %f",
					evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
			if (len > 0) {
				printk("%s\n", log_buf);
			}
			break;
		}

		case LTE_LC_EVT_RRC_UPDATE:
			printk("RRC mode: %s\n",
				evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
				"Connected" : "Idle");
			break;

		case LTE_LC_EVT_CELL_UPDATE:
			printk("LTE cell changed: Cell ID: %d, Tracking area: %d\n",
				evt->cell.id, evt->cell.tac);
			break;

		default:
			break;
	}
}

