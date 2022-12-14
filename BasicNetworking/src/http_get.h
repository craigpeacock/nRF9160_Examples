
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/net/socket.h>

void nslookup(const char * hostname, struct addrinfo **results);
void print_addrinfo_results(struct addrinfo **results);
void http_get(void);