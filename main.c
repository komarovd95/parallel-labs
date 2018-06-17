#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <unistd.h>
#include "omp.h"
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>

#define EPS 1e-6
#define NUMBER_OF_TESTS 10
#define USEC_IN_SECOND 1000000L
#define MAX_EXECUTION_TIME 10000000000L // 10s
#define A 637.0
#define CHUNK_SIZE 64

#ifndef _OPENMP
double omp_get_wtime(void) {
    struct timeval wtime;
    gettimeofday(&wtime, NULL);
    return wtime.tv_sec + ((double) wtime.tv_usec) / USEC_IN_SECOND;
}

int omp_get_num_procs(void) {
    return 1;
}
#endif

/**
 * Generates a random array of given size and seed.
 * An array has even distribution between (1; A).
 *
 * @param n size of generated array
 * @return random array
 */
double *generate_m1(int n);

/**
 * Maps values of given array: m1[i] = sqrt(m1[i] / E)
 *
 * @param n size of given array
 * @param m1 an array
 */
void map_m1(int n, double *m1);

/**
 * Maps values of given array: m1[i] = sqrt(m1[i] / E)
 *
 * @param n size of given array
 * @param queue the OpenCL command queue
 * @param kernel the kernel that will be executed
 * @param buffer the buffer that contains values of M1
 */
void map_m1_CL(int n, cl_command_queue queue, cl_kernel kernel, cl_mem buffer);

/**
 * Generates a random array of given size and seed.
 * An array has even distribution between (A; 10A).
 *
 * @param n size of generated array
 * @return random array
 */
double *generate_m2(int n);

/**
 * Maps values of given array: m2[i] = abs(tan(m2[i] + m2[i-1]))
 *
 * @param n size of given array
 * @param m2 an array
 */
void map_m2(int n, double *m2);

/**
 * Maps values of given array: m2[i] = abs(tan(m2[i] + m2[i-1]))
 *
 * @param n size of given array
 * @param queue the OpenCL command queue
 * @param copy_kernel the kernel that copies values from M2 to TMP
 * @param sum_kernel the kernel that sums values of M2 and TMP
 * @param map_kernel the kernel that maps values of M2
 * @param m2 the buffer that contains values of M2
 * @param tmp the buffer that contains values of TMP
 */
void map_m2_CL(int n, cl_command_queue queue, cl_kernel copy_kernel, cl_kernel sum_kernel, cl_kernel map_kernel, cl_mem m2, cl_mem tmp);

/**
 * Merges values of given arrays into M2 array.
 * Applies function to values while merging: f(a, b) = a * b
 *
 * @param n size of M2 array
 * @param m1 a M1 array
 * @param m2 a M2 array
 */
void merge(int n, double *m1, double *m2);

/**
 * Merges values of given arrays into M2 array.
 * Applies function to values while merging: f(a, b) = a * b
 *
 * @param n size of M2 array
 * @param queue the OpenCL command queue
 * @param kernel the kernel that merges values of M1 and M2
 * @param m1 a M1 buffer
 * @param m2 a M2 buffer
 */
void merge_CL(int n, cl_command_queue queue, cl_kernel kernel, cl_mem m1, cl_mem m2);

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
 * @param n size of given array
 * @param array an array
 */
void rs_sort(int n, double *array);

/**
 * Finds minimal positive element of given array.
 *
 * @param n size of given array
 * @param array sorted array
 * @return minimal positive element or 0.0 if no positive elements found
 */
double min_positive(int n, double *array);

/**
 * Reduces given array into single double value by function:
 * f(a[i]) = f(a[i-1]) + ([a / min] % 2 == 0 ? sin(a / min) : 0.0)
 *
 * @param n size of given array
 * @param array an array
 * @param min minimal value for dividing, must be positive
 * @return reduced sum
 */
double reduce(int n, double *array, double min);

/**
 * Apply sum function to pairs of zipped arrays.
 * 
 * @param left result array
 * @param right an array
 * @param len size of zipped arrays
 */
void zip_sum(double *left, double *right, int len);

/**
 *  Does work for given steps:
 *  - generating arrays
 *  - mapping them
 *  - sorting and merging
 *  - reducing into result
 *
 * @param n size of work (N)
 * @return reduced result of work
 */
double do_work(int n);

/**
 * Prepares OpenCL context.
 *
 * @param context the OpenCL context that will be prepared
 * @param queue the command queue that will be prepared
 */
void prepare_opencl_context(cl_context *context, cl_command_queue *queue);

/**
 * Builds OpenCL program.
 *
 * @param program the program that will be built
 * @param context the prepared OpenCL context
 * @param file_name the name of file that contains OpenCL kernels
 */
void build_opencl_program(cl_program *program, cl_context context, char *file_name);

/**
 * Reads the given file.
 * 
 * @param filename the name of file that will be read
 */
char *read_file(char *filename);

int main(int argc, char* argv[]) {
    int n;
    int i;
    long int best_time;
    double expected_result;

    n = atoi(argv[1]);

    best_time = MAX_EXECUTION_TIME;
    for (i = 0; i < NUMBER_OF_TESTS; ++i) {
        double work_result;

        work_result = do_work(n);

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

double do_work(int n) {
    double x;
    int progress;
    cl_context context;
    cl_command_queue queue;
    cl_program program;

    progress = 0;

#ifdef USE_OPENCL
    prepare_opencl_context(&context, &queue);
    build_opencl_program(&program, context, "kernels.cl");
#endif

    #pragma omp parallel sections default(none) shared(n,x,progress,program,queue,context)
    {
        #pragma omp section
        {
#ifdef _OPENMP
            while (progress < 100) {
                sleep(1);
#ifdef DEBUG                
                printf("Progress: %d\n", progress);
#endif                
            }
#endif            
        }
        #pragma omp section
        {
            double *m1, *m2;
            int m1_size, m2_size;
            double min_value;
            double start, end;
            cl_kernel map_m1_kernel, map_m2_copy_kernel, map_m2_sum_kernel, map_m2_kernel, merge_kernel;
            cl_mem cl_m1, cl_m2, cl_tmp;

            start = omp_get_wtime();

            m1_size = n;
            m2_size = n / 2;

            m1 = generate_m1(m1_size);
            progress += 5;
            m2 = generate_m2(m2_size);
            progress += 5;

#ifdef USE_OPENCL
            map_m1_kernel = clCreateKernel(program, "map_m1", NULL);
            map_m2_copy_kernel = clCreateKernel(program, "map_m2_copy", NULL);
            map_m2_sum_kernel = clCreateKernel(program, "map_m2_sum", NULL);
            map_m2_kernel = clCreateKernel(program, "map_m2", NULL);
            merge_kernel = clCreateKernel(program, "merge", NULL);

            cl_m1 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(double) * m1_size, NULL, NULL);
            cl_m2 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(double) * m2_size, NULL, NULL);
            cl_tmp = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(double) * m2_size, NULL, NULL);
#endif

#ifdef USE_OPENCL
            clEnqueueWriteBuffer(queue, cl_m1, CL_TRUE, 0, sizeof(double) * m1_size, m1, 0, NULL, NULL);
            map_m1_CL(m1_size, queue, map_m1_kernel, cl_m1);
            clEnqueueReadBuffer(queue, cl_m1, CL_TRUE, 0, sizeof(double) * m1_size, m1, 0, NULL, NULL);
#else
            map_m1(m1_size, m1);
#endif
            progress += 10;

#ifdef USE_OPENCL
            clEnqueueWriteBuffer(queue, cl_m2, CL_TRUE, 0, sizeof(double) * m2_size, m2, 0, NULL, NULL);
            map_m2_CL(m2_size, queue, map_m2_copy_kernel, map_m2_sum_kernel, map_m2_kernel, cl_m2, cl_tmp);
            clEnqueueReadBuffer(queue, cl_m2, CL_TRUE, 0, sizeof(double) * m2_size, m2, 0, NULL, NULL);
#else
            map_m2(m2_size, m2);
#endif
            progress += 10;

#ifdef USE_OPENCL
            merge_CL(m2_size, queue, merge_kernel, cl_m1, cl_m2);
            clEnqueueReadBuffer(queue, cl_m2, CL_TRUE, 0, sizeof(double) * m2_size, m2, 0, NULL, NULL);
#else
            merge(m2_size, m1, m2);
#endif
            progress += 10;

            rs_sort(m2_size, m2);
            progress += 50;

            min_value = min_positive(m2_size, m2);
            if (min_value == 0.0) {
                printf("Failed to find minimal value :(\n");
                exit(100);
            }

            x = reduce(m2_size, m2, min_value);
            progress += 10;
            end = omp_get_wtime();
            printf("%ld;%f\n", (long) ((end - start) * USEC_IN_SECOND), x);
            free(m1);
            free(m2);
        }
    }
    return x;
}

double *generate_m1(int n) {
    int i;
    double *array;

    array = malloc(sizeof(double) * n);
    #pragma omp parallel for default(none) private(i) shared(array,n) schedule(runtime)
    for (i = 0; i < n; ++i) {
        double val;
        unsigned int seed;
        seed = i * i;
        val = (double) rand_r(&seed);
        val /= (double) RAND_MAX;
        val *= (A - 1.0);
        val += 1.0;
        array[i] = val;
    }
    return array;
}

void map_m1_CL(int n, cl_command_queue queue, cl_kernel kernel, cl_mem buffer) {
    cl_event event;
    cl_ulong startCl, endCl;
    double start, end;
    size_t size;

    size = n;
    start = omp_get_wtime();
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer);
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &size, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);
    end = omp_get_wtime();

    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &startCl, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &endCl, NULL);

    printf("%ld;%ld;", (long) ((end - start) * USEC_IN_SECOND), (endCl - startCl) / 1000);
}

void map_m1(int n, double *m1) {
    int i;
    double start, end;

    start = omp_get_wtime();
    #pragma omp parallel for default(none) private(i) shared(m1,n) schedule(runtime)
    for (i = 0; i < n; ++i) {
        m1[i] = sqrt(m1[i] / M_E);
    }
    end = omp_get_wtime();
    printf("%ld;", (long) ((end - start) * USEC_IN_SECOND));
}

double *generate_m2(int n) {
    int i;
    double *array;

    array = malloc(sizeof(double) * n);
    #pragma omp parallel for default(none) private(i) shared(array,n) schedule(runtime)
    for (i = 0; i < n; ++i) {
        double val;
        unsigned int seed;
        seed = i * i * i;
        val = (double) rand_r(&seed);
        val /= (double) RAND_MAX;
        val *= (A * 9.0);
        val += A;
        array[i] = val;
    }
    return array;
}

void map_m2(int n, double *m2) {
    int i;
    double *temp;
    double start, end;

    start = omp_get_wtime();
    temp = malloc(sizeof(double) * n);
    #pragma omp parallel for default(none) private(i) shared(temp,m2,n) schedule(runtime)
    for (i = 0; i < n; ++i) {
        temp[i] = m2[i];
    }

    zip_sum(m2 + 1, temp, n - 1);

    #pragma omp parallel for default(none) private(i) shared(m2,n) schedule(runtime)
    for (i = 0; i < n; ++i) {
        m2[i] = fabs(tan(m2[i]));
    }
    free(temp);
    end = omp_get_wtime();
    printf("%ld;", (long) ((end - start) * USEC_IN_SECOND));
}

void map_m2_CL(int n, cl_command_queue queue, cl_kernel copy_kernel, cl_kernel sum_kernel, cl_kernel map_kernel, cl_mem m2, cl_mem tmp) {
    cl_ulong startCl, endCl, elapsed;
    cl_event event;
    double start, end;
    size_t size;

    size = n;
    elapsed = 0;
    start = omp_get_wtime();

    clSetKernelArg(copy_kernel, 0, sizeof(cl_mem), &m2);
    clSetKernelArg(copy_kernel, 1, sizeof(cl_mem), &tmp);
    clEnqueueNDRangeKernel(queue, copy_kernel, 1, NULL, &size, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &startCl, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &endCl, NULL);
    elapsed += (endCl - startCl);

    clSetKernelArg(sum_kernel, 0, sizeof(cl_mem), &m2);
    clSetKernelArg(sum_kernel, 1, sizeof(cl_mem), &tmp);
    clEnqueueNDRangeKernel(queue, sum_kernel, 1, NULL, &size, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &startCl, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &endCl, NULL);
    elapsed += (endCl - startCl);

    clSetKernelArg(map_kernel, 0, sizeof(cl_mem), &m2);
    clEnqueueNDRangeKernel(queue, map_kernel, 1, NULL, &size, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &startCl, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &endCl, NULL);
    elapsed += (endCl - startCl);
    end = omp_get_wtime();

    printf("%ld;%ld;", (long) ((end - start) * USEC_IN_SECOND), elapsed / 1000);
}

void zip_sum(double *left, double *right, int len) {
    int i;
    #pragma omp parallel for default(none) private(i) shared(left,right,len) schedule(runtime)
    for (i = 0; i < len; ++i) {
        left[i] += right[i];
    }
}

void merge(int n, double *m1, double *m2) {
    int i;
    double start, end;

    start = omp_get_wtime();
    #pragma omp parallel for default(none) private(i) shared(m1,m2,n) schedule(runtime)
    for (i = 0; i < n; ++i) {
        m2[i] *= m1[i];
    }
    end = omp_get_wtime();
    printf("%ld;", (long) ((end - start) * USEC_IN_SECOND));
}

void merge_CL(int n, cl_command_queue queue, cl_kernel kernel, cl_mem m1, cl_mem m2) {
    cl_event event;
    cl_ulong startCl, endCl;
    double start, end;
    size_t size;

    size = n;
    start = omp_get_wtime();
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &m1);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &m2);
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &size, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);
    end = omp_get_wtime();

    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &startCl, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &endCl, NULL);

    printf("%ld;%ld;", (long) ((end - start) * USEC_IN_SECOND), (endCl - startCl) / 1000);
}

void rs_sort(int n, double *array) {
    int chunks;
    int chunk_size;
    int chunk_n;
    double *temp;

    chunk_size = CHUNK_SIZE;
    chunks = (n + chunk_size - 1) / chunk_size;

    #pragma omp parallel for default(none) private(chunk_n) shared(array,chunk_size,chunks,n) schedule(runtime)
    for (chunk_n = 0; chunk_n < chunks; ++chunk_n) {
        int subsize;
        subsize = n - chunk_size * chunk_n;
        subsize = subsize < chunk_size ? subsize : chunk_size;
        sort(subsize, array + chunk_n * chunk_size);
    }

    temp = malloc(sizeof(double) * n);
    for (; chunk_size < n; chunk_size *= 2) {
        chunks = (n + chunk_size - 1) / chunk_size;
        #pragma omp parallel for default(none) private(chunk_n) shared(array,chunk_size,chunks,n,temp) schedule(runtime)
        for (chunk_n = 0; chunk_n < chunks; chunk_n += 2) {
            int temp_size;
            int i, j, k;
            int chunk_0, chunk_1, chunk_2;

            chunk_0 = chunk_n * chunk_size;
            chunk_1 = chunk_0 + chunk_size;
            chunk_2 = chunk_1 + chunk_size;

            temp_size = chunk_0;
            for (j = chunk_0, k = chunk_1; j < n && k < n && j < chunk_1 && k < chunk_2;) {
                if (array[j] < array[k]) {
                    temp[temp_size++] = array[j++];
                } else {
                    temp[temp_size++] = array[k++];
                }
            }
            while (j < n && j < chunk_1) {
                temp[temp_size++] = array[j++];
            }
            while (k < n && k < chunk_2) {
                temp[temp_size++] = array[k++];
            }
            for (i = chunk_0; i < temp_size; ++i) {
                array[i] = temp[i];
            }
        }
    }
    free(temp);
}

/**
 * Sorting algorithm invariants will be broken while parallelizing
 */
void sort(int n, double *array) {
    int i, j;
    double key;
    int lo, hi, m;

    for (i = 1; i < n; ++i) {
        key = array[i];
        for (j = i - 1; j >= 0 && array[j] > key; --j) {
            array[j + 1] = array[j];
        }
        array[j + 1] = key;
    }
}

/**
 * This method has O(1) complexity in common case
 */
double min_positive(int n, double *array) {
    int i;
    for (i = 0; i < n; ++i) {
        if (array[i] != 0.0) {
            return array[i];
        }
    }
    return 0.0;
}

double reduce(int n, double *array, double min) {
    int i;
    double sum;

    sum = 0.0;
    #pragma omp parallel for default(none) private(i) shared(array,min,n) reduction(+:sum) schedule(runtime)
    for (i = 0; i < n; ++i) {
        double value;

        value = array[i] / min;
        if (!(((int) value) % 2)) {
            sum += sin(value);
        }
    }
    return sum;
}

void prepare_opencl_context(cl_context *context, cl_command_queue *queue) {
    cl_platform_id platform_id;
    cl_device_id device_id;

    clGetPlatformIDs(1, &platform_id, NULL);
    clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_CPU, 1, &device_id, NULL);
    *context = clCreateContext(NULL, 1, &device_id, NULL, NULL, NULL);
    *queue = clCreateCommandQueue(*context, device_id, CL_QUEUE_PROFILING_ENABLE, NULL);
}

void build_opencl_program(cl_program *program, cl_context context, char *file_name) {
    char *source;

    source = read_file(file_name);
    *program = clCreateProgramWithSource(context, 1, (const char **)&source, NULL, NULL);
    clBuildProgram(*program, 0, NULL, NULL, NULL, NULL);
}

char *read_file(char *filename) {
    FILE *f;
    size_t file_size;
    char *result;

    f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    result = (char *) malloc(file_size + 1);
    fread(result, 1, file_size, f);
    fclose(f);

    result[file_size] = '\0';

    return result;
}
