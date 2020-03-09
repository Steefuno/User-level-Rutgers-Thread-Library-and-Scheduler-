#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../rpthread.h"

int cc = 0;
pthread_mutex_t* mutex1;

void* count(void* argv) {
	int i = 0;

	pthread_mutex_lock(mutex1);
	while (i < (int)argv) {
		cc++;
		i++;
	}
	pthread_mutex_unlock(mutex1);

	printf(">Exiting\n");
	pthread_exit(NULL);
}

int main(int argc, char **argv) {
	pthread_t thread1, thread2;
	printf(">Init Mutex\n");
	pthread_mutex_init(mutex1, NULL);

	printf(">Creating1\n");
	pthread_create(&thread1, NULL, count, (void*)1000000);
	printf(">Creating2\n");
	pthread_create(&thread2, NULL, count, (void*)1000000);

	printf(">Joining1\n");
	pthread_join(thread1, NULL);
	printf(">Joined1\n");

	printf(">Joining2\n");
	pthread_join(thread2, NULL);
	printf(">Joined2\n");

	printf(">Ended on %d\n", cc);

	return 0;
}
