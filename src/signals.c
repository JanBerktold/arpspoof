#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>

#include "signals.h"

uint8_t quit_program = 0;

static void quit_handler(int signo) {
	printf("recieved termination request\n");
	quit_program = 1;
}

void setup_signals() {
	if (signal(SIGINT, quit_handler) == SIG_ERR) {
		perror("signal err");		
	}
};
