#include <stdio.h>
#include <stdlib.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>

#define WORKER_THREADS 8
#define BUFFER_SIZE 1500

struct thread_info {
	int id;
	char status;

	pthread_t thread;
	pthread_cond_t cond;
	pthread_mutex_t mutex;

	void* buffer;
	int length;
} *threads[WORKER_THREADS];

void *worker(void* thread_p) {
	struct thread_info* thread = thread_p;
	struct ether_header* eh = (struct ether_header*)thread->buffer;

	while (1) {
		thread->status = 1;
		pthread_cond_wait(&thread->cond, &thread->mutex);
		printf("handle pkg with length %i from thread %i\n", thread->length, thread->id);
	}
}

struct thread_info* get_ready_thread() {
	int i = 0;
	while (threads[i]->status == 0) {
		i = (i + 1)  % WORKER_THREADS;
	}
	return threads[i];
}

int main() {

	for (int i = 0; i < WORKER_THREADS; i++) {
		threads[i] = malloc(sizeof(struct thread_info));
		threads[i]->id = i;
		threads[i]->status = 0;

		// init buffer
		threads[i]->buffer = malloc(BUFFER_SIZE);
		if (threads[i]->buffer  < 0) {
			printf("failed to create buffer");
			exit(1);
		}

		// init thread
		threads[i]->cond = (pthread_cond_t) PTHREAD_COND_INITIALIZER;		
		pthread_mutex_init (&threads[i]->mutex, NULL);

		// spawn thread
		int rc = pthread_create(&threads[i]->thread, NULL, worker, threads[i]);
		if (rc < 0) {
			printf("failed to create thread");
			exit(1);
		}
	}

	int main_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	printf("socket id %i\n", main_socket);

	// packet dispatcher loop
	while (1) {
		struct thread_info* thread = get_ready_thread();
		thread->status = 0;

		thread->length = recv(main_socket, thread->buffer, BUFFER_SIZE, 0);
		if (thread->length < 0) {
			exit(1);
		}

		pthread_cond_signal(&thread->cond);	
	}
	pthread_exit(NULL);
}
