/* 
 * nRF9160 Key Management Example
 * API Reference:
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/modem/modem_key_mgmt.html
 *
 * Demo to see if we can store configuration settings (board configuration, hardware 
 * version, board serial number) in with credentials. Credentials are loaded
 * over the AT host port during manufacturer and board settings could be loaded using 
 * the same method.
 * 
 * Example includes AT Host Library on UART0 for Credential Management:
 * To list credentials:
 *  AT%CMNG=1
 * 
 * Read credentials:
 *  AT%CMNG=2,<SECURITY TAG>,<TYPE>
 * 
 * Write Credential:
 *  AT%CMNG=0,<SECURITY TAG>,<TYPE>,<CONTENT>
 *  AT%CMNG=0,1,0,"HWVER:2.1"
 *    (Type 0 = Root CA certificate (ASCII text))
 * 
 * Should read security tag 0, Root CA certificate to output:
 * *** Booting Zephyr OS build v2.7.99-ncs1-1  ***
 * nRF9160 Key Management Example
 * Success, Credential 9 bytes long
 * HWVER:2.1
 *
 */

#include <zephyr.h>
#include <stdio.h>
#include <drivers/uart.h>
#include <string.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_key_mgmt.h>

#define MAX_BUFFER_LENGTH 	1500

void main(void)
{
	int ret;
	
	nrf_sec_tag_t	SecurityTag = 1;
	int8_t 			buffer[MAX_BUFFER_LENGTH];
	size_t 			bufferlen = MAX_BUFFER_LENGTH;

	printk("nRF9160 Key Management Example\n");

	ret = modem_key_mgmt_read(SecurityTag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &buffer, &bufferlen);
	if (ret == 0) {
		printk("Success, Credential %d bytes long\n", bufferlen);
		buffer[bufferlen] = 0;
		printk("%s\n\n",buffer);
	} else {
		switch (ret) {
			case (-ENOBUFS):
				printk("Internal buffer too small\n");
				break;
			case (-ENOMEM):
				printk("Provided buffer is too small for result data\n");
				break;
			case (-ENOENT):
				printf("No credential associated with the given sec_tag and cred_type\n");
				break;
			case (-EPERM):
				printf("Insufficient permissions\n");
				break;
			default:
				printk("Error reading credential, %d\n", ret);
				break;
		}
	}
}