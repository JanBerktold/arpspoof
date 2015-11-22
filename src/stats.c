#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <inttypes.h>

#include "stats.h"
#include "sys/eventfd.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define timestamp_ms(timeval) ((timeval).tv_sec*1000 + (timeval).tv_usec/1000)

struct stat_pair* init_stat_pair() {
	struct stat_pair* result = malloc(sizeof(struct stat_pair));
	result->eventfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    	if (result->eventfd < 0) {
        	perror("eventfd failed.");
    	}
	result->value = 0;
	return result;
};

struct stat_collection* init_stat_collection() {
	struct stat_collection* result = malloc(sizeof(struct stat_collection));
	result->ipv4_packet = init_stat_pair();
	result->arp_packet = init_stat_pair();
	result->unknown_packet = init_stat_pair();
	result->dhcp_data = init_stat_pair();
	return result;
};

void stat_collection_to_readset(struct stat_collection* stats, fd_set* set, int* highest_descriptor) {
	FD_ZERO(set);
	FD_SET(stats->ipv4_packet->eventfd, set);
	FD_SET(stats->arp_packet->eventfd, set);
	FD_SET(stats->dhcp_data->eventfd, set);

	*highest_descriptor = max(stats->ipv4_packet->eventfd, stats->arp_packet->eventfd);
	*highest_descriptor = max(*highest_descriptor, stats->dhcp_data->eventfd);
	(*highest_descriptor)++;
};

void print_stat_collection_csv(struct stat_collection* stats, struct timeval* time) {
	// time, ipv4_packet, arp_packet, unknown_packet
	printf("%li,%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
		timestamp_ms(*time),
		stats->ipv4_packet->value,
		stats->arp_packet->value,
		stats->unknown_packet->value
	);
};
