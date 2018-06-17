#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>

#define EPS 1e-6
#define NUMBER_OF_TESTS 10
#define USEC_IN_SECOND 1000000L
#define MAX_EXECUTION_TIME 10000000000L // 10s
#define A 637.0
#define CHUNK_SIZE 64
#define STEPS 9

/**
 * Range structure.
 * 
 * @param start start of range
 * @param end end of range
 */
typedef struct rng {
	int start;
	int end;
} RANGE;

/**
 * Does work for given steps:
 *  - generating arrays
 *  - mapping them
 *  - sorting and merging
 *  - reducing into result
 *
 * @return reduced result of work
 */
double do_work();

/**
 * Thread routine.
 *
 * @param arg thread ID
 */
void *thread_routine(void *arg);

/**
 * Progress routine.
 */
void *progress_routine(void *arg);

/**
 * Calculates bunch of work for the given thread.
 *
 * @param tid thread ID
 * @param n total size of work
 * @param range calculating range
 */
void calculate_range(int tid, int n, RANGE *range);

/**
 * Generates part of a random array of size N.
 * An array has even distribution between (1; A).
 *
 * @param tid thread ID
 * @return result of work
 */
int generate_m1(int tid);

/**
 * Generates part of a random array of size N / 2.
 * An array has even distribution between (A; 10A).
 *
 * @param tid thread ID
 * @return result of work
 */
int generate_m2(int tid);

/**
 * Maps values of part of M1: m1[i] = sqrt(m1[i] / E)
 *
 * @param tid thread ID
 * @return result of work
 */
int map_m1(int tid);

/**
 * Maps values of part of M2: m2[i] = abs(tan(m2[i] + m2[i-1]))
 *
 * @param tid thread ID
 * @return result of work
 */
int map_m2(int tid);

/**
 * Maps values of part of M2: m2[i] = abs(tan(m2[i] + m2[i-1]))
 *
 * @param tid thread ID
 * @return result of work
 */
int map_m2_copy(int tid);

/**
 * Merges values of parts of M1 and M2 into M2.
 * Applies function to values while merging: f(a, b) = a * b
 *
 * @param tid thread ID
 * @return result of work
 */
int merge(int tid);

/**
 * Sorts given array using Insertion Sort algorithm.
 *
 * @see https://en.wikipedia.org/wiki/Insertion_sort
 * @param n size of given array
 * @param array an array
 */
void sort(int n, double *array);

/**
 * Sorts given array using parallelization.
 * Inspired by Regular Sampling Parallel Sort Algorithm.
 *
 * @param tid thread ID
 * @return result of work
 */
int chunk_sort(int tid);

/**
 * Merges sorted chunks.
 *
 * @param tid thread ID
 */
void sort_merge(int tid);

/**
 * Posts result of sorted chunks merge.
 *
 * @param size the size of merged part of array
 */
void post_merge(int size);

/**
 * Finds minimal positive element of M2.
 *
 * @return minimal positive element or 0.0 if no positive elements found
 */
double min_positive();

/**
 * Reduces part of M2 into single double value by function:
 * f(a[i]) = f(a[i-1]) + ([a / min] % 2 == 0 ? sin(a / min) : 0.0)
 *
 * @param n size of given array
 * @param array an array
 * @param min minimal value for dividing, must be positive
 * @return reduced sum
 */
int reduce(int tid, double min, double *x);

/**
 * Returns current wall clock time in seconds.
 *
 * @return wall clock time
 */
double get_wtime(void);

int n, k;
int chunk_size;
double *m1, *m2, *tmp;
int m1_size, m2_size;

int *threads;
pthread_barrier_t create_barrier;
pthread_barrier_t generate_barrier;
pthread_barrier_t map_barrier;
pthread_barrier_t m2_zip_barrier;
pthread_barrier_t merge_barrier;
pthread_barrier_t sort_barrier;
pthread_barrier_t sort_merge_barrier;
pthread_barrier_t reduce_barrier;

int merged;
pthread_mutex_t merge_mutex;
pthread_cond_t merge_cond;

volatile int sort_chunk_size;
int *thread_states;
int offset_marks[STEPS] = { 0 };
pthread_mutex_t *range_mutexes;

volatile int progress;

int main(int argc, char* argv[]) {
	int i;
	double expected_result;

	n = atoi(argv[1]);
	k = atoi(argv[2]);
	chunk_size = atoi(argv[3]);

    for (i = 0; i < NUMBER_OF_TESTS; ++i) {
        double work_result;

        work_result = do_work();

        if (i == 0) {
            expected_result = work_result;
        } else if (fabs(expected_result - work_result) > EPS) {
            printf("\nFailed to verify result of work: expected=%.5f actual=%.5f", 
                expected_result, work_result);
            return 1; 
        }
#ifdef DEBUG
        printf("Work result = %lf\n", work_result);
#endif
    }
    return 0;
}

double do_work() {
	int i;
	pthread_t *pthreads;
	pthread_attr_t attr;
	double start, end;
	double start_g, end_g;
	double x;

	start_g = get_wtime();

	m1_size = n;
    m2_size = n / 2;

    m1 = malloc(sizeof(double) * m1_size);
    m2 = malloc(sizeof(double) * m2_size);
    tmp = malloc(sizeof(double) * m2_size);

	pthreads = malloc(sizeof(pthread_t) * (k + 1));
	threads = malloc(sizeof(int) * k);

	range_mutexes = malloc(sizeof(pthread_mutex_t) * STEPS);
	thread_states = malloc(sizeof(int) * k);

	pthread_barrier_init(&create_barrier, NULL, k + 2);
	pthread_barrier_init(&generate_barrier, NULL, k + 1);
	pthread_barrier_init(&map_barrier, NULL, k + 1);
	pthread_barrier_init(&m2_zip_barrier, NULL, k);
	pthread_barrier_init(&merge_barrier, NULL, k + 1);
	pthread_barrier_init(&sort_barrier, NULL, k + 1);
	pthread_barrier_init(&sort_merge_barrier, NULL, k);
	pthread_barrier_init(&reduce_barrier, NULL, k + 1);

	merged = 0;
	pthread_mutex_init(&merge_mutex, NULL);
	pthread_cond_init(&merge_cond, NULL);

	for (i = 0; i < STEPS; ++i) {
		pthread_mutex_init(&range_mutexes[i], NULL);
		offset_marks[i] = 0;
	}

	for (i = 0; i < k; ++i) {
		int *arg;
		arg = malloc(sizeof(int));
		*arg = i;
		pthread_attr_init(&attr);
		pthread_create(&pthreads[i], &attr, thread_routine, arg);
	}

	pthread_attr_init(&attr);
	pthread_create(&pthreads[k], &attr, progress_routine, NULL);

	progress = 0;
    sort_chunk_size = CHUNK_SIZE;
	pthread_barrier_wait(&create_barrier);
	start = get_wtime();
	pthread_barrier_wait(&generate_barrier);
	progress += 10;
	end = get_wtime();
	printf("%f;", end - start);
	start = end;
	pthread_barrier_wait(&map_barrier);
	progress += 20;
	end = get_wtime();
	printf("%f;", end - start);
	start = end;
	pthread_barrier_wait(&merge_barrier);
	progress += 10;
	end = get_wtime();
	printf("%f;", end - start);
	start = end;
	pthread_barrier_wait(&sort_barrier);
	progress += 50;
	end = get_wtime();
	printf("%f;", end - start);
	start = end;
	pthread_barrier_wait(&reduce_barrier);
	progress += 10;
	end = get_wtime();
	printf("%f;", end - start);

	x = 0.0;
	for (i = 0; i < k; ++i) {
		void *res;
		pthread_join(pthreads[i], &res);
		x += *((double *) res);
	}

	pthread_barrier_destroy(&create_barrier);
	pthread_barrier_destroy(&generate_barrier);
	pthread_barrier_destroy(&map_barrier);
	pthread_barrier_destroy(&m2_zip_barrier);
	pthread_barrier_destroy(&merge_barrier);
	pthread_barrier_destroy(&sort_barrier);
	pthread_barrier_destroy(&reduce_barrier);

	pthread_mutex_destroy(&merge_mutex);
	pthread_cond_destroy(&merge_cond);
	for (i = 0; i < STEPS; ++i) {
		pthread_mutex_destroy(&range_mutexes[i]);
	}

	free(m1);
	free(m2);
	free(tmp);
	free(threads);
	free(range_mutexes);
	free(thread_states);

	end_g = get_wtime();
	printf("%f;%f\n", end_g - start_g, x);

	pthread_join(pthreads[k], NULL);
	free(pthreads);

	return x;
}

void *thread_routine(void *arg) {
	int tid;
	double min_val;
	double *x;

	tid = *((int *) arg);
	x = malloc(sizeof(double));

	pthread_barrier_wait(&create_barrier);

	thread_states[tid] = 0;
	threads[tid] = 0;
	while (generate_m1(tid));
	thread_states[tid] = 1;
	threads[tid] = 0;
	while (generate_m2(tid));
	pthread_barrier_wait(&generate_barrier);

	thread_states[tid] = 2;
	threads[tid] = 0;
	while (map_m1(tid));
	thread_states[tid] = 3;
	threads[tid] = 0;
	while (map_m2_copy(tid));
	pthread_barrier_wait(&m2_zip_barrier);
	thread_states[tid] = 4;
	threads[tid] = 0;
	while (map_m2(tid));
	pthread_barrier_wait(&map_barrier);

	thread_states[tid] = 5;
	threads[tid] = 0;
	while (merge(tid));
	pthread_barrier_wait(&merge_barrier);

	thread_states[tid] = 6;
	threads[tid] = 0;
	while (chunk_sort(tid));
	pthread_barrier_wait(&sort_merge_barrier);
	thread_states[tid] = 7;
	threads[tid] = 0;
	sort_merge(tid);
	pthread_barrier_wait(&sort_barrier);

	min_val = min_positive();
	thread_states[tid] = 8;
	threads[tid] = 0;
	*x = 0.0;
	while (reduce(tid, min_val, x));
	pthread_barrier_wait(&reduce_barrier);
#ifdef DEBUG
	printf("Work is done: tid=%d, x=%f\n", tid, *x);
#endif
	pthread_exit(x);
}

void *progress_routine(void *arg) {
	pthread_barrier_wait(&create_barrier);
    while (progress < 100) {
        sleep(1);
#ifdef OUT_PROGRESS                
        printf("Progress: %d\n", progress);
#endif                
    }
}

void calculate_range(int tid, int n, RANGE *range) {
#ifdef SCHED_STATIC	
	int done_jobs;
	int start;

	done_jobs = threads[tid];
	start = done_jobs * k * chunk_size + tid * chunk_size;

	range->start = start;
	if (start > n) {
		range->end = start;
	} else if (start + chunk_size > n) {
		range->end = n;
	} else {
		range->end = start + chunk_size;
	}
	threads[tid]++;
#endif

#ifdef SCHED_DYNAMIC
	int i;
	int start;
	int state;

	state = thread_states[tid];
	pthread_mutex_lock(&range_mutexes[state]);
	start = offset_marks[state];
	range->start = start;
	if (start >= n) {
		range->end = start;
	} else if (start + chunk_size > n) {
		range->end = n;
		offset_marks[state] = n;
	} else {
		range->end = start + chunk_size;
		offset_marks[state] += chunk_size;
	}
	pthread_mutex_unlock(&range_mutexes[state]);
#endif

#ifdef SCHED_GUIDED
	int i;
	int start;
	int state;
	int guided_chunk_size;

	state = thread_states[tid];
	pthread_mutex_lock(&range_mutexes[state]);
	start = offset_marks[state];
	guided_chunk_size = (n - start) / k;
	guided_chunk_size = (guided_chunk_size > chunk_size) ? guided_chunk_size : chunk_size;
	range->start = start;
	if (start >= n) {
		range->end = start;
	} else if (start + guided_chunk_size > n) {
		range->end = n;
		offset_marks[state] = n;
	} else {
		range->end = start + guided_chunk_size;
		offset_marks[state] += guided_chunk_size;
	}
	pthread_mutex_unlock(&range_mutexes[state]);
#endif
}

void calculate_merge(int tid, int chunk_s, RANGE *range) {
	int total_chunks;

	if (chunk_s != sort_chunk_size) {
		range->start = -1;
		range->end = -1;
		pthread_mutex_unlock(&merge_mutex);
		return;
	}
	total_chunks = (m2_size + chunk_s - 1) / chunk_s;
	calculate_range(tid, (total_chunks + 1) / 2, range);
}

int generate_m1(int tid) {
    int i;
    RANGE range;
    double val;
    unsigned int seed;

    calculate_range(tid, m1_size, &range);
#ifdef DEBUG
    printf("GenM1: tid=%d, start=%d, end=%d\n", tid, range.start, range.end);
#endif  
    for (i = range.start; i < range.end; ++i) {
        seed = i * i;
        val = (double) rand_r(&seed);
        val /= (double) RAND_MAX;
        val *= (A - 1.0);
        val += 1.0;
        m1[i] = val;
    }
    return range.end < m1_size;
}

int generate_m2(int tid) {
    int i;
    RANGE range;
    double val;
    unsigned int seed;

    calculate_range(tid, m2_size, &range);
#ifdef DEBUG
    printf("GenM2: tid=%d, start=%d, end=%d\n", tid, range.start, range.end);
#endif     
    for (i = range.start; i < range.end; ++i) {
        seed = i * i * i;
        val = (double) rand_r(&seed);
        val /= (double) RAND_MAX;
        val *= (A * 9.0);
        val += A;
        m2[i] = val;
    }
    return range.end < m2_size;
}

int map_m1(int tid) {
    int i;
    RANGE range;

    calculate_range(tid, m1_size, &range);
#ifdef DEBUG
    printf("MapM1: tid=%d, start=%d, end=%d\n", tid, range.start, range.end);
#endif
    for (i = range.start; i < range.end; ++i) {
        m1[i] = sqrt(m1[i] / M_E);
    }
    return range.end < m1_size;
}

int map_m2_copy(int tid) {
	int i;
	RANGE range;

	calculate_range(tid, m2_size, &range);
#ifdef DEBUG
    printf("MapM2 copy: tid=%d, start=%d, end=%d\n", tid, range.start, range.end);
#endif
    for (i = range.start; i < range.end; ++i) {
    	tmp[i] = m2[i];
    }
    return range.end < m2_size;
}

int map_m2(int tid) {
	int i;
    RANGE range;

    calculate_range(tid, m2_size, &range);
#ifdef DEBUG
    printf("MapM2: tid=%d, start=%d, end=%d\n", tid, range.start, range.end);
#endif
    for (i = range.start; i < range.end; ++i) {
    	double val;
    	val = m2[i];
    	if (i) {
    		val += tmp[i - 1];
    	}
    	m2[i] = fabs(tan(val));
    }
    return range.end < m2_size;
}

int merge(int tid) {
	int i;
    RANGE range;

    calculate_range(tid, m2_size, &range);
#ifdef DEBUG
    printf("Merge: tid=%d, start=%d, end=%d\n", tid, range.start, range.end);
#endif
    for (i = range.start; i < range.end; ++i) {
        m2[i] *= m1[i];
    }
    return range.end < m2_size;
}

int chunk_sort(int tid) {
	int i;
	RANGE range;
	int total_chunks;
	int sort_chunk;

	sort_chunk = CHUNK_SIZE;
	total_chunks = (m2_size + sort_chunk - 1) / sort_chunk;

	calculate_range(tid, total_chunks, &range);
#ifdef DEBUG
    printf("Chunk Sort: tid=%d, start=%d, end=%d\n", tid, range.start, range.end);
#endif
    for (i = range.start; i < range.end; ++i) {
        int subsize;
        subsize = m2_size - sort_chunk * i;
        subsize = subsize < sort_chunk ? subsize : sort_chunk;
        sort(subsize, m2 + i * sort_chunk);
    }
    return range.end < total_chunks;
}

void sort_merge(int tid) {
	int sort_chunk;
	RANGE range;
	int c;

    sort_chunk = CHUNK_SIZE;

	for (; sort_chunk < m2_size; sort_chunk *= 2) {
		int total_merged;
    	total_merged = 0;
    	do
    	{
	    	calculate_merge(tid, sort_chunk, &range);	
	#ifdef DEBUG
	    printf("Sort merge: tid=%d, start=%d, end=%d, chunk_size=%d\n", tid, range.start, range.end, sort_chunk);
	#endif
	    	for (c = range.start; c < range.end; ++c) {
	    		int temp_size;
	            int i, j, k;
	            int chunk_0, chunk_1, chunk_2;

	            chunk_0 = c * 2 * sort_chunk;
	            chunk_1 = chunk_0 + sort_chunk;
	            chunk_2 = chunk_1 + sort_chunk;

	            temp_size = chunk_0;
	            for (j = chunk_0, k = chunk_1; j < m2_size && k < m2_size && j < chunk_1 && k < chunk_2;) {
	                if (m2[j] < m2[k]) {
	                    tmp[temp_size++] = m2[j++];
	                } else {
	                    tmp[temp_size++] = m2[k++];
	                }
	            }
	            while (j < m2_size && j < chunk_1) {
	                tmp[temp_size++] = m2[j++];
	            }
	            while (k < m2_size && k < chunk_2) {
	                tmp[temp_size++] = m2[k++];
	            }
	#ifdef DEBUG
	    printf("Sort merge: temp_size=%d, chunk_0=%d, chunk_1=%d, chunk_2=%d\n", temp_size, chunk_0, chunk_1, chunk_2);
	#endif
	            for (i = chunk_0; i < temp_size; ++i) {
	                m2[i] = tmp[i];
	            }
	            total_merged += (temp_size - chunk_0);
	    	}
	    } while (range.start < range.end);

    	if (total_merged) {
    		post_merge(total_merged);
    	}
    	threads[tid] = 0;
    }
}

void sort(int s, double *array) {
    int i, j;
    double key;
    int lo, hi, m;

    for (i = 1; i < s; ++i) {
        key = array[i];
        for (j = i - 1; j >= 0 && array[j] > key; --j) {
            array[j + 1] = array[j];
        }
        array[j + 1] = key;
    }
}

void post_merge(int size) {
	pthread_mutex_lock(&merge_mutex);
	merged += size;
#ifdef DEBUG
		printf("Sort merging update: merged=%d\n", merged);
#endif	
	if (m2_size - merged) {
		pthread_cond_wait(&merge_cond, &merge_mutex);
	} else {
		merged = 0;
		sort_chunk_size *= 2;
		offset_marks[7] = 0; // sort-merge step offset
#ifdef DEBUG
		printf("Sort merging next step!\n");
#endif		
		pthread_cond_broadcast(&merge_cond);
	}
	pthread_mutex_unlock(&merge_mutex);
}

/**
 * This method has O(1) complexity in common case
 */
double min_positive() {
    int i;
    for (i = 0; i < m2_size; ++i) {
        if (m2[i] != 0.0) {
            return m2[i];
        }
    }
    return 0.0;
}

int reduce(int tid, double min, double *x) {
	int i;
	RANGE range;
    double sum;

    sum = *x;
    calculate_range(tid, m2_size, &range);
    for (i = range.start; i < range.end; ++i) {
        double value;

        value = m2[i] / min;
        if (!(((int) value) % 2)) {
            sum += sin(value);
        }
    }
    *x = sum;
    return range.end < m2_size;
}

double get_wtime(void) {
    struct timeval wtime;
    gettimeofday(&wtime, NULL);
    return wtime.tv_sec + ((double) wtime.tv_usec) / USEC_IN_SECOND;
}
