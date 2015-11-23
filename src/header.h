#ifndef HEADER_DEFINED
#define HEADER_DEFINED

#include <stdint.h>
#include <asm/byteorder.h>
#include <pthread.h>

#define MAC_ADDR_LENGTH 6

struct thread_info {
	int id;
	pthread_t thread;

	int socket;
	int if_index;

	void* buffer;
	size_t length;

	void* send_buffer;
	size_t send_length;
};

int thread_send_buffer(struct thread_info*);

struct ether_header {
    uint8_t    dest_host[MAC_ADDR_LENGTH];
    uint8_t    source_host[MAC_ADDR_LENGTH];
    uint16_t   protocol;
};

struct arp_data {
	uint16_t hardware_type;
	uint16_t protocol_type;
	uint8_t hardware_addr_length;
	uint8_t protocol_addr_length;
	uint16_t operation;
	uint8_t first_addr_byte;
	// followed by hardware & protocol addr
};

// handle with variable sized addresses
uint8_t* arp_sender_hardware_addr(struct arp_data*);
uint8_t* arp_sender_protocol_addr(struct arp_data*);
uint8_t* arp_target_hardware_addr(struct arp_data*);
uint8_t* arp_target_protocol_addr(struct arp_data*);

struct ip_header {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	unsigned char header_length:4, version:4;	
#elif defined (__BIG_ENDIAN_BITFIELD)	
	unsigned char version:4, header_length:4;
#else
#error "Neither endian type is defined."	
#endif
	uint8_t type_of_service;      
	uint16_t total_length; 	
	uint16_t id;  
	uint16_t flags;
 	uint8_t time_to_live;
	uint8_t protocol;
	uint16_t header_checksum;  
	uint32_t source;       
	uint32_t dest;
};

struct udp_header {
        uint16_t src_port;
        uint16_t dest_port;
        uint16_t length;
        uint16_t checksum;
};


void print_ether_header(struct ether_header*);
void print_arp_data(struct arp_data*);
void print_ip_header(struct ip_header*);
void print_udp_header(struct udp_header*);
//void print_tcp_header(struct tcp_header* header);

#endif
