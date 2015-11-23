#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <netinet/if_ether.h>
#include <netinet/ip.h>

#include <sys/socket.h>
#include <sys/poll.h>
#include <pthread.h>
#include <string.h>

#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>	/* The L2 protocols */

#include <net/if.h>
#include <sys/eventfd.h>
#include <sys/time.h>

#include "signals.h"
#include "header.h"
#include "dhcp.h"
#include "arp.h"
#include "stats.h"

#define timestamp_ms(timeval) ((timeval).tv_sec*1000 + (timeval).tv_usec/1000)

#define WORKER_THREADS 8
#define BUFFER_SIZE 1500

int INTERFACE_INDEX;
char* INTERFACE_NAME;
struct stat_collection* stats;
struct thread_info* threads[WORKER_THREADS];

void *worker(void* thread_p) {
	struct thread_info* thread = thread_p;

	struct ether_header* eth_hdr = (struct ether_header*)thread->buffer;
	struct ip_header* ip_hdr = (struct ip_header*)(thread->buffer + sizeof(struct ether_header));
	struct udp_header* udp_hdr = (struct udp_header*)(ip_hdr + ip_hdr->header_length);

	thread->socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	
	if (thread->socket < 0) {
		printf("error creating socket");
		exit(1);
	}

	// set socket group
	uint16_t id = 129;
	int val = PACKET_FANOUT_FLAG_DEFRAG | (PACKET_FANOUT_LB << 16) | id;
	int err = setsockopt(thread->socket, SOL_PACKET, PACKET_FANOUT, &val, sizeof(val));

	// set promsic mode
	struct packet_mreq mr;
	memset(&mr, 0, sizeof(mr));
	mr.mr_ifindex = INTERFACE_INDEX;
	mr.mr_type = PACKET_MR_PROMISC;
	err += setsockopt(thread->socket, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr));

	// bind socket to interface
	err += setsockopt(thread->socket, SOL_SOCKET, SO_BINDTODEVICE, INTERFACE_NAME, strlen(INTERFACE_NAME));

	if (err < 0) {
		printf("error setting opt");
		exit(1);
	}

	while (1) {
		thread->length = recv(thread->socket, thread->buffer, BUFFER_SIZE, 0);
		switch (ntohs(eth_hdr->protocol)) {
			case ETH_P_IP:
				eventfd_write(stats->ipv4_packet->eventfd, 1);
				if (ip_hdr->protocol == 17) {
					/*print_ether_header(eth_hdr);
					print_ip_header(ip_hdr);
					print_udp_header(udp_hdr);*/
				}
				if (handle_if_dhcp_packet(ip_hdr, thread->length-sizeof(struct ether_header))) {
					eventfd_write(stats->dhcp_data->eventfd, 1);
					continue;
				}
				//printf("handle IPv4 pkg with length %zd and secret protocol %hu from thread %i\n", thread->length, ip_hdr->protocol, thread->id);
				break;
			case ETH_P_ARP:
				handle_arp_packet(thread);
				eventfd_write(stats->arp_packet->eventfd, 1);
				break;
			default:
				eventfd_write(stats->unknown_packet->eventfd, 1);
				//printf("attempt to handle pkg with unknown type %hu\n", ntohs(eth_hdr->protocol));
		}
	}
}

int find_interface(char* name) {
	struct if_nameindex* start_iface = if_nameindex();
	struct if_nameindex* current_iface = start_iface;
	int retval = -1;

	while (current_iface->if_index > 0) {
		if (strcmp(current_iface->if_name, name)) {
			INTERFACE_INDEX = current_iface->if_index;		
			INTERFACE_NAME = name;
			retval = 0;
			break;
		}
		current_iface++;
	}

	if_freenameindex(start_iface);
	return retval;
}

int main(int argc, char *argv[]) {
	setup_signals();
	if (find_interface(argv[1]) < 0 ) {
		printf("did not find interface with name %s\n", argv[1]);
		exit(1);
	}
	
	stats = init_stat_collection();
	for (int i = 0; i < WORKER_THREADS; i++) {
		threads[i] = malloc(sizeof(struct thread_info));
		memset(threads[i], 0, sizeof(struct thread_info));
		threads[i]->id = i;
	
		threads[i]->if_index = INTERFACE_INDEX;
		// init buffer
		threads[i]->buffer = malloc(BUFFER_SIZE);
		threads[i]->send_buffer = malloc(BUFFER_SIZE);
		if (threads[i]->buffer < 0 || threads[i]->send_buffer < 0) {
			printf("failed to create buffer\n");
			exit(1);
		}
		memset(threads[i]->buffer, 0, BUFFER_SIZE);
		memset(threads[i]->send_buffer, 0, BUFFER_SIZE);

		// spawn thread
		int rc = pthread_create(&threads[i]->thread, NULL, worker, threads[i]);
		if (rc < 0) {
			printf("failed to create thread\n");
			exit(1);
		}
	}

	int amount;
	int highest_descriptor;
	fd_set readfds;
	struct timeval last_print, current;
	struct timeval timeout;

#define update(pair, set) \
	if (FD_ISSET(pair->eventfd, set)) { \
		eventfd_t value; \
		int result = eventfd_read(pair->eventfd, &value); \
		if (result < -1) { \
			perror("error reading eventfd"); \
		} \
		pair->value += value; \
	} \

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	gettimeofday(&last_print, NULL);
	stat_collection_to_readset(stats, &readfds, &highest_descriptor);
	while (quit_program == 0) {
		amount = select(highest_descriptor, &readfds, NULL, NULL, NULL);
		if (amount > 0)  {
			update(stats->ipv4_packet, &readfds);
			update(stats->arp_packet, &readfds);
			update(stats->unknown_packet, &readfds);
			update(stats->dhcp_data, &readfds);
		}

		gettimeofday(&current, NULL);
		uint64_t diff = timestamp_ms(current) - timestamp_ms(last_print);
		if (diff >= 1000) {
			print_stat_collection_csv(stats, &current);
			last_print = current;
		}
		
		// set timeout for select
		timeout.tv_sec = 0;
		timeout.tv_usec = (diff + 1000) * 1000;
		//printf("amount %i and timeout %li\n", amount, timestamp_ms(timeout));
	}

	pthread_exit(NULL);
}
