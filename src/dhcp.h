#ifndef DHCP_H
#define DHCP_H

#include <stdlib.h>
#include "header.h"

char handle_if_dhcp_packet(struct ip_header* ip_hdr, size_t length);

uint32_t* request_ip_address();
void free_ip_address(uint32_t* address);

#endif
