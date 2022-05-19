#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(CONFIG_NRF_MODEM_LIB)
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_info.h>
#include <nrf_modem.h>
#endif
#include <sys/reboot.h>
#include <dfu/mcuboot.h>

void nrf_modem_lib_dfu_handler(void)
{
	int err;

	err = nrf_modem_lib_get_init_ret();

	switch (err) {
		case MODEM_DFU_RESULT_OK:
			printk("Modem update suceeded, reboot\n");
			sys_reboot(SYS_REBOOT_COLD);
			break;

		case MODEM_DFU_RESULT_UUID_ERROR:
		case MODEM_DFU_RESULT_AUTH_ERROR:
			printk("Modem update failed, error: %d\n", err);
			printk("Modem will use old firmware\n");
			sys_reboot(SYS_REBOOT_COLD);
			break;

		case MODEM_DFU_RESULT_HARDWARE_ERROR:
		case MODEM_DFU_RESULT_INTERNAL_ERROR:
			printk("Modem update malfunction, error: %d, reboot\n", err);
			sys_reboot(SYS_REBOOT_COLD);
			break;
			
		default:
			break;
	}
}
