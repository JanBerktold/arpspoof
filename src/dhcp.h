int handle_dhcp_packet(void* buffer, size_t length);
uint32_t* request_ip_address();
void free_ip_address(uint32_t* address);
