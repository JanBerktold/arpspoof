#ifndef STATS_H_HEADER
#define STATS_H_HEADER

#include <sys/select.h>

struct stat_pair {
	int eventfd;
	uint64_t value;
};

struct stat_collection {
	// top level request types
	struct stat_pair* ipv4_packet;
	struct stat_pair* arp_packet;
	struct stat_pair* unknown_packet;

	struct stat_pair* dhcp_data;
};

struct stat_collection* init_stat_collection();
void stat_collection_to_readset(struct stat_collection* stats, fd_set* set, int* highest_descriptor);
 
void print_stat_collection_csv(struct stat_collection*, struct timeval*);
#endif
