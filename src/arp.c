#include <stdio.h>
#include "header.h"
#include <netinet/in.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void ip_to_mac_addr(uint8_t* ip, uint8_t* mac) {
	memset(mac, 0, MAC_ADDR_LENGTH);
	memcpy(mac, ip+2, 2);
	memcpy(mac+2, ip, 2);
}


int handle_arp_packet(struct thread_info* thread) {

	struct ether_header* ether = (struct ether_header*) thread->buffer;
	struct arp_data* arp = (struct arp_data*) (thread->buffer + sizeof(struct ether_header));
	
	struct ether_header* send_ether = (struct ether_header*) thread->send_buffer;
	struct arp_data* send_arp = (struct arp_data*) (thread->send_buffer + sizeof(struct ether_header));

	printf("%" PRIu16 "\n", ntohs(arp->operation));
	switch (ntohs(arp->operation)) {
	case 1:
		printf("request\n");
		print_ether_header(ether);
		print_arp_data(arp);
		
		// copy recieved data for ETHERNET and ARP packet
		// we are just recycling the old data because that saves
		// us a lot of struct fields which would have to be set
		// otherwise
		memcpy(thread->send_buffer, thread->buffer, thread->length);

		ip_to_mac_addr(
			arp_target_protocol_addr(arp),
			send_ether->source_host);
		
		ip_to_mac_addr(
			arp_target_protocol_addr(arp),
			arp_sender_hardware_addr(send_arp));
	
		memcpy(
			send_ether->dest_host,
			ether->source_host,
			MAC_ADDR_LENGTH);

		// set ARP REPLY type
		send_arp->operation = ntohs(2);

		// set sender in ARP reply
		memcpy(
			arp_sender_protocol_addr(send_arp),
			arp_target_protocol_addr(arp),
			arp->protocol_addr_length);
		
		// set target in ARP reply
		memcpy(
			arp_target_hardware_addr(send_arp),
			arp_sender_hardware_addr(arp),
			arp->hardware_addr_length),
		
		memcpy(
			arp_target_protocol_addr(send_arp),
			arp_sender_protocol_addr(arp),
			arp->hardware_addr_length),

		thread->send_length = sizeof(struct ether_header) + sizeof(struct arp_data);
		thread_send_buffer(thread);
		printf("our reply\n");
		print_ether_header(send_ether);
		print_arp_data(send_arp);
		break;
	case 2:
		break;
	};

	return 0;
}

