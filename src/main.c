#include <stdio.h>
#include <stdlib.h>

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

#include "dhcp.h"
#include "arp.h"

#define WORKER_THREADS 8
#define BUFFER_SIZE 1500

int INTERFACE_INDEX;
char* INTERFACE_NAME;

struct thread_info {
	int id;
	pthread_t thread;

	void* buffer;
	size_t length;
} *threads[WORKER_THREADS];

void *worker(void* thread_p) {
	struct thread_info* thread = thread_p;

	struct ethhdr* eth_hdr = (struct ethhdr*)thread->buffer;
	struct iphdr* ip_hdr = (struct iphdr*)(thread->buffer + sizeof(struct ethhdr));

	int t_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	
	if (t_socket < 0) {
		printf("error creating socket");
		exit(1);
	}

	// set socket group
	uint16_t id = 129;
	int val = PACKET_FANOUT_FLAG_DEFRAG | (PACKET_FANOUT_LB << 16) | id;
	int err = setsockopt(t_socket, SOL_PACKET, PACKET_FANOUT, &val, sizeof(val));

	// set promsic mode
	struct packet_mreq mr;
	memset(&mr, 0, sizeof(mr));
	mr.mr_ifindex = INTERFACE_INDEX;
	mr.mr_type = PACKET_MR_PROMISC;
	err += setsockopt(t_socket, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr));

	// bind socket to interface
	err += setsockopt(t_socket, SOL_SOCKET, SO_BINDTODEVICE, INTERFACE_NAME, strlen(INTERFACE_NAME));

	if (err < 0) {
		printf("error setting opt");
		exit(1);
	}

	while (1) {
		thread->length = recv(t_socket, thread->buffer, BUFFER_SIZE, 0);
		switch (ntohs(eth_hdr->h_proto)) {
			case ETH_P_IP:
				printf("handle IPv4 pkg with length %zd and secret protocol %hu from thread %i\n", thread->length, ip_hdr->protocol, thread->id);
				break;
			case ETH_P_ARP:
				printf("handle arp pkg with length %zd from thread %i\n", thread->length, thread->id);
				handle_arp_packet(thread->buffer, thread->length);
				break;
			default:
				printf("attempt to handle pkg with unknown type %hu\n", ntohs(eth_hdr->h_proto));
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
	if (find_interface(argv[1]) < 0 ) {
		printf("did not find interface with name %s\n", argv[1]);
		exit(1);
	}

	for (int i = 0; i < WORKER_THREADS; i++) {
		threads[i] = malloc(sizeof(struct thread_info));
		threads[i]->id = i;

		// init buffer
		threads[i]->buffer = malloc(BUFFER_SIZE);
		if (threads[i]->buffer < 0) {
			printf("failed to create buffer\n");
			exit(1);
		}

		// spawn thread
		int rc = pthread_create(&threads[i]->thread, NULL, worker, threads[i]);
		if (rc < 0) {
			printf("failed to create thread\n");
			exit(1);
		}
	}

	//TODO: implement CTRL-C stop
	while (1) {
	}

	pthread_exit(NULL);
}
