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

void set_ftrace_parameters()
{
	int ret;
	int pid = getpid();
	char buff[200];

	printf("Setting ftrace parameters...\n");
	ret = system("echo 0 > /sys/kernel/debug/tracing/tracing_on");
	if (ret)
		goto error;
	ret = system("> /sys/kernel/debug/tracing/trace");
	if (ret)
		goto error;
	ret = system("echo function_graph > /sys/kernel/debug/tracing/current_tracer");
	if (ret)
		goto error;
	ret = system("echo __do_page_fault > /sys/kernel/debug/tracing/set_ftrace_filter");
	if (ret)
		goto error;
	/* set buffer to 1GB. Make sure to clean it up while exiting. */
	ret = system("echo 1048576 > /sys/kernel/debug/tracing/buffer_size_kb");
	/* copy pid into the buffer */
	sprintf(buff, "echo %d > /sys/kernel/debug/tracing/set_ftrace_pid", pid);
	ret = system(buff);
	if (ret)
		goto error;

	ret = system("echo 1 > /sys/kernel/debug/tracing/tracing_on");
	if (ret)
		goto error;

	printf("ftrace configured. executing the worklod...\n ");
	return;
error:
	printf("Error occured while setting ftrace parameters\n");
	exit(EXIT_FAILURE);
}

void collect_and_clear_ftrace()
{
	int ret;
	system("tail -n+5 /sys/kernel/debug/tracing/trace > log");
	ret = system("echo 0 > /sys/kernel/debug/tracing/tracing_on");
	if (ret)
		goto error;
	ret = system("> /sys/kernel/debug/tracing/trace");
	if (ret)
		goto error;
	ret = system("echo 1408 > /sys/kernel/debug/tracing/buffer_size_kb");
	if (ret)
		goto error;
	system("sed 's/^.......//' log > tmp; rm -r log;\
			cat tmp | grep __do_page_fault > log; rm -f tmp");
	printf("\n");
	system("total=$(awk '{sum+=$1;n++} END { if(n>0) print(sum)}' log);\
				echo Total Page Fault Time: $total microsecs");
	system("avg=$(awk '{sum+=$1;n++} END { if(n>0) print(sum/n)}' log);\
				echo Average Page Fault Time: $avg microsecs");

	printf("\n");
	printf("ftrace parameters cleared successfully...\n");
	return;
error:
	printf("Error occured while clearing ftrace parameters");
	exit(EXIT_FAILURE);
}

void print_ftrace_summary()
{

}

void *alloc_mem(void *arg)
{
	int fd, nr_pages, iter = 0;
	char *tmp, *start;

	while(iter < iterations) {
		/* Allocate required bytes */
#if 0
		tmp = mmap(NULL, bytes_per_thread, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if (tmp == MAP_FAILED) {
			printf("mmap failed\n");
			return NULL;
		}
#endif
		tmp = malloc(bytes_per_thread);
		if (!tmp)
			exit(EXIT_FAILURE);

		start = tmp;
		nr_pages = bytes_per_thread/4096;
		/* access page to force memory allocation via page-fault */
		while(nr_pages--) {
			*start = 'A';
			/* move to the next page. */
			start += 4096;
		}
#if 0
		if (munmap(tmp, bytes_per_thread) == 1)
			perror("error unmapping the file\n");
#endif
		free(tmp);

		iter++;
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	int i, c, pid, ret;
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

	set_ftrace_parameters();
	for (i=0; i < nr_threads; i++)
		pthread_create(&pthread[i], NULL, alloc_mem, NULL);

	for (i=0; i < nr_threads; i++)
		pthread_join(pthread[i],NULL);

	collect_and_clear_ftrace();
	print_ftrace_summary();
	/* This is precautionary. */
	system("echo 1408 > /sys/kernel/debug/tracing/buffer_size_kb");
	printf("Exiting successfully.\n");
	return 0;
}
