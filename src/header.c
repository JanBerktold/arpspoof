#include <stdio.h>
#include <stdlib.h>
#include <header.h>
#include <netinet/in.h>

void print_ether_header(struct ether_header* header) {
        printf("Ether Header [Layer 2]\n");
        printf("\tDestination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                header->dest_host[0], header->dest_host[1],
                header->dest_host[2], header->dest_host[3],
                header->dest_host[4], header->dest_host[5]);
        printf("\tSource MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                header->source_host[0], header->source_host[1],
                header->source_host[2], header->source_host[3],
                header->source_host[4], header->source_host[5]);
	printf("\tProtocol: %u\n", header->protocol);
};

uint8_t* arp_sender_hardware_addr(struct arp_data* arp) {
	return &arp->first_addr_byte;
};

uint8_t* arp_sender_protocol_addr(struct arp_data* arp) {
	return &arp->first_addr_byte + arp->hardware_addr_length;
};

uint8_t* arp_target_hardware_addr(struct arp_data* arp) {
	return &arp->first_addr_byte + arp->hardware_addr_length + arp->protocol_addr_length;
};

uint8_t* arp_target_protocol_addr(struct arp_data* arp) {
	return &arp->first_addr_byte + 2 * arp->hardware_addr_length + arp->protocol_addr_length;
};

char* sprint_hardware_addr(struct arp_data* arp, uint8_t* addr) {
	char* result = malloc(arp->hardware_addr_length*3);
	uint8_t* pointer = addr;
	for (int i = 0; i < arp->hardware_addr_length; i++) {
		sprintf(result + (i * 3), "%02x", *pointer);
		pointer++;
		*(result + ((i+1) * 3)-1) = ':';
	}
	result[arp->hardware_addr_length*3-1] = '\0';
	return result;
}

char* sprint_protocol_addr(struct arp_data* arp, uint8_t* addr) {
	char* result = malloc(arp->protocol_addr_length*4);
	uint8_t* pointer = addr;
	for (int i = 0; i < arp->protocol_addr_length; i++) {
		sprintf(result + (i * 4), "%03hu", *pointer);
		*(result +((i+1) * 4)-1) = '.';
		pointer++;
	}
	result[arp->protocol_addr_length*4-1] = '\0';
	return result;
}

void print_arp_data(struct arp_data* arp) {
	printf("ARP Packet [Layer2a]\n\tHardware Type: %d\n\tProtocol Type: %d\n\tHardware Addr Length: %d\n\tProtocol Addr Length: %d\n\tOperation: %d\n\tSender Hardware Addr: %s\n\tSender Protocol Addr: %s\n\tTarget Hardware Addr: %s\n\tTarget Protocol Addr: %s\n", 
		ntohs(arp->hardware_type), ntohs(arp->protocol_type),
		arp->hardware_addr_length, arp->protocol_addr_length,
		ntohs(arp->operation),
		sprint_hardware_addr(arp, arp_sender_hardware_addr(arp)),
		sprint_protocol_addr(arp, arp_sender_protocol_addr(arp)),
		sprint_hardware_addr(arp, arp_target_hardware_addr(arp)),
		sprint_protocol_addr(arp, arp_target_protocol_addr(arp))
	);
};

void print_ip(int ip)
{
    unsigned char bytes[4];
    bytes[3] = ip & 0xFF;
    bytes[2] = (ip >> 8) & 0xFF;
    bytes[1] = (ip >> 16) & 0xFF;
    bytes[0] = (ip >> 24) & 0xFF;	
    printf("%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);        
};

void print_ip_header(struct ip_header* header) {
	printf("IP Header [Layer 3]\n");
	printf("\tVersion: %d\n\tHeader length: %d bytes\n\tType of Service: %d\n\tTotal Length: %d\n\tIdentification: %d\n",
		header->version, header->header_length*4, 
		header->type_of_service, ntohs(header->total_length), 
		ntohs(header->id)
	);

	printf("\tTime to live: %d\n", header->time_to_live);
	printf("\tProtocol: %d\n", header->protocol);
	printf("\tHeader Checksum: %d\n", ntohs(header->header_checksum));
	printf("\tSource: ");
	print_ip(header->source);
	printf("\n\tDest: ");
	print_ip(header->dest);
	printf("\n");
};

void print_udp_header(struct udp_header* header) {
	printf("UDP Header [Layer 4]\n\tSource Port: %d\n\tDest Port: %d\n\tLength: %d\n\tChecksum: %d\n",
		header->src_port, header->dest_port, header->length, header->checksum);
};
