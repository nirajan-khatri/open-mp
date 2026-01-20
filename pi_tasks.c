#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

// Cache line size to prevent false sharing
#define CACHE_LINE_SIZE 64

// Padded integer to avoid false sharing between threads
typedef struct {
    int count;
    char padding[CACHE_LINE_SIZE - sizeof(int)];
} padded_int;

// Padded double to avoid false sharing between threads
typedef struct {
    double value;
    char padding[CACHE_LINE_SIZE - sizeof(double)];
} padded_double;

// Hash function from specification PDF (Page 7)
unsigned long hash(unsigned long x) {
    x ^= (x >> 21);
    x *= 2654435761UL;
    x ^= (x >> 13);
    x *= 2654435761UL;
    x ^= (x >> 17);
    return x;
}

// Concatenate function from specification PDF (Page 7)
unsigned concatenate(unsigned x, unsigned y) {
    unsigned pow = 10;
    while (y >= pow)
        pow *= 10;
    return x * pow + y;
}

// my_rand function from specification PDF (Page 7)
unsigned long my_rand(unsigned long* state, unsigned long lower, unsigned long upper) {
    *state ^= *state >> 12;
    *state ^= *state << 25;
    *state ^= *state >> 27;
    unsigned long result = (*state * 0x2545F4914F6CDD1DULL);
    unsigned long range = (upper > lower) ? (upper - lower) : 0UL;
    return (range > 0) ? (result % range + lower) : lower;
}

// Compute π using Riemann sum (midpoint rule) - integral method from exercises
// π = integral from 0 to 1 of 4/(1+x²) dx
double compute_pi(unsigned long precision) {
    double sum = 0.0;
    double step = 1.0 / (double)precision;
    
    #pragma omp simd reduction(+:sum)
    for (unsigned long i = 0; i < precision; i++) {
        double x = (i + 0.5) * step;  // Midpoint of each interval
        sum += 4.0 / (1.0 + x * x);
    }
    
    return sum * step;
}

// Recursive function to spawn tasks
void spawn_pi_task(unsigned long task_seed, int *tasks_created, int num_tasks, 
                   unsigned long lower, unsigned long upper, padded_double *thread_pi, 
                   padded_int *tasks_per_thread, int num_threads) {
    
    // Check if we've reached the limit
    int current_count;
    #pragma omp atomic capture
    current_count = ++(*tasks_created);
    
    if (current_count > num_tasks) {
        return; // Exceeded limit, don't process this task
    }
    
    int thread_id = omp_get_thread_num();
    
    // Compute precision for this task using deterministic seed
    unsigned long state = task_seed;
    unsigned long precision = my_rand(&state, lower, upper);
    
    // Compute pi
    double pi_value = compute_pi(precision);
    
    // Update thread-local accumulators (no atomic needed - each thread owns its slot)
    thread_pi[thread_id].value += pi_value;
    tasks_per_thread[thread_id].count++;
    
    // Determine how many new tasks to spawn (1-4)
    unsigned long spawn_state = hash(task_seed);
    int num_new_tasks = my_rand(&spawn_state, 1, 5);  // Returns 1-4
    
    // Spawn new child tasks
    for (int i = 0; i < num_new_tasks; i++) {
        // Check if we can spawn more tasks
        int check_count;
        #pragma omp atomic read
        check_count = *tasks_created;
        
        if (check_count < num_tasks) {
            // Create unique seed for child task using hash and concatenate
            unsigned long child_seed = hash(task_seed * concatenate(i + 1, thread_id + 1));
            
            #pragma omp task firstprivate(child_seed)
            {
                spawn_pi_task(child_seed, tasks_created, num_tasks, lower, upper, 
                             thread_pi, tasks_per_thread, num_threads);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <num_tasks> <num_threads> <lower> <upper> <seed>\n", argv[0]);
        return 1;
    }
    
    // Parse command-line arguments
    int num_tasks = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    unsigned long lower = atol(argv[3]);
    unsigned long upper = atol(argv[4]);
    unsigned long seed = atol(argv[5]);
    
    // Set number of OpenMP threads
    omp_set_num_threads(num_threads);
    
    // Validate input
    if (num_tasks <= 0 || num_threads <= 0 || upper <= lower) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return 1;
    }
    
    // Start timing
    double start_time = omp_get_wtime();
    
    // Shared variables
    int tasks_created = 0;
    
    // Use padded arrays to prevent false sharing
    padded_double *thread_pi = (padded_double*) calloc(num_threads, sizeof(padded_double));
    padded_int *tasks_per_thread = (padded_int*) calloc(num_threads, sizeof(padded_int));
    
    if (tasks_per_thread == NULL || thread_pi == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    
    // Create initial task region
    #pragma omp parallel
    {
        #pragma omp single
        {
            // Spawn the initial task
            #pragma omp task
            {
                spawn_pi_task(seed, &tasks_created, num_tasks, lower, upper, 
                             thread_pi, tasks_per_thread, num_threads);
            }
            
            // Wait for all tasks to complete
            #pragma omp taskwait
        }
    }
    
    // Sum up thread-local contributions
    double total_pi = 0.0;
    for (int i = 0; i < num_threads; i++) {
        total_pi += thread_pi[i].value;
    }
    
    // Calculate average (only count valid tasks)
    int valid_tasks = (tasks_created <= num_tasks) ? tasks_created : num_tasks;
    double average_pi = total_pi / valid_tasks;
    
    // Output results (per spec: timing ends AFTER printing output)
    printf("Average pi: %.10f\n", average_pi);
    for (int i = 0; i < num_threads; i++) {
        printf("Thread %d computed %d tasks\n", i, tasks_per_thread[i].count);
    }
    
    // End timing after printing results (per specification)
    double end_time = omp_get_wtime();
    printf("Execution took %.4f s\n", end_time - start_time);
    
    // Clean up
    free(tasks_per_thread);
    free(thread_pi);
    
    return 0;
}

