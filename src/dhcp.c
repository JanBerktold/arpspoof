#include "dhcp.h"
#include "header.h"

#include <stdlib.h>
#include <netinet/in.h>

inline char handle_if_dhcp_packet(struct ip_header* ip_hdr, size_t length) {
	struct udp_header* udp_hdr = (struct udp_header*)(ip_hdr + ip_hdr->header_length);
	if (ip_hdr->protocol == IPPROTO_UDP && udp_hdr->dest_port == 67 && udp_hdr->src_port == 68) {
		// handle DHCP packet
		return 1;
	}
	return 0;
}

