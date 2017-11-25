#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

int nr_threads = 1;
int iterations = 1;
unsigned long memory = 1;
unsigned long bytes_per_thread = 1;

void *alloc_mem(void *arg)
{
	int fd, nr_pages, iter = 0;
	char *tmp, *start;

	while(iter < iterations) {
		/* Allocate required bytes */
		tmp = mmap(NULL, bytes_per_thread, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if (tmp == MAP_FAILED) {
			printf("mmap failed\n");
			return NULL;
		}
		start = tmp;
		nr_pages = bytes_per_thread/4096;
		/* access page to force memory allocation via page-fault */
		while(nr_pages--) {
			*start = 'A';
			/* move to the next page. */
			start += 4096;
		}
		if (munmap(tmp, bytes_per_thread) == 1)
			perror("error unmapping the file\n");

		iter++;
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	int i, c;
	pthread_t *pthread;

	while ((c = getopt(argc, argv, "m:t:i:")) != -1) {
		switch(c) {
			case 'm':
				memory = atoi(optarg);
				break;
			case 't':
				nr_threads = atoi(optarg);
				break;
			case 'i':
				iterations = atoi(optarg);
				break;
			default:
				printf("Usage: %s [-m memoryGB] [-t nr_threads] [-i iterations]\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	pthread = (pthread_t *)malloc(sizeof(pthread_t) * nr_threads);
	if (!pthread) {
		printf("Error while allocating pthreads\n");
		exit(EXIT_FAILURE);
	}
	bytes_per_thread = (memory*1024*1024*1024) / nr_threads;
	printf("Running with the following configuration...\n");
	printf("Memory: \t\t%ldGB\n", memory);
	printf("Threads: \t\t%d\n", nr_threads);
	printf("Iterations: \t\t%d\n", iterations);
	printf("Bytes Per Thread: \t%ld\n", bytes_per_thread);

	for (i=0; i < nr_threads; i++)
		pthread_create(&pthread[i], NULL, alloc_mem, NULL);

	for (i=0; i < nr_threads; i++)
		pthread_join(pthread[i],NULL);

	printf("Exiting successfully...\n");
	return 0;
}
