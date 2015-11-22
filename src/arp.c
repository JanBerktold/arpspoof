#include <stdio.h>
#include "header.h"
#include <netinet/in.h>

int handle_arp_packet(struct arp_data* arp) {
	print_arp_data(arp);
	return 0;
}

