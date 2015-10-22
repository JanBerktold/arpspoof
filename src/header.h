#ifndef HEADER_DEFINED
#define HEADER_DEFINED

#include <stdint.h>

#define MAC_ADDR_LENGTH 6

struct ether_header {
    uint8_t    dest_host[MAC_ADDR_LENGTH];
    uint8_t    source_host[MAC_ADDR_LENGTH];
    uint16_t   protocol;
};

struct ip_header {
    unsigned char version:4, header_length:4;
    uint8_t type_of_service;      
    uint16_t total_length; 
    uint16_t id;  
    uint16_t flags;  //2 Byte
    uint8_t time_to_live;       //1 Byte
    uint8_t protocol;         //1 Byte
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

#endif
