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

// Hash function
unsigned long hash(unsigned long x) {
    x ^= (x >> 21);
    x *= 2654435761UL;
    x ^= (x >> 13);
    x *= 2654435761UL;
    x ^= (x >> 17);
    return x;
}

// Concatenate function
unsigned concatenate(unsigned x, unsigned y) {
    unsigned pow = 10;
    while (y >= pow)
        pow *= 10;
    return x * pow + y;
}

// my_rand function
unsigned long my_rand(unsigned long* state, unsigned long lower, unsigned long upper) {
    *state ^= *state >> 12;
    *state ^= *state << 25;
    *state ^= *state >> 27;
    unsigned long result = (*state * 0x2545F4914F6CDD1DULL);
    unsigned long range = (upper > lower) ? (upper - lower) : 0UL;
    return (range > 0) ? (result % range + lower) : lower;
}

// Initialize heatmap with random values (NUMA-aware: parallel for first-touch)
unsigned long* initialize_heatmap(int rows, int cols, unsigned long seed, unsigned long lower, unsigned long upper) {
    // Allocate flat 1D array to represent 2D matrix
    unsigned long *heatmap = (unsigned long*) malloc(rows * cols * sizeof(unsigned long));
    
    if (heatmap == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(1);
    }
    
    // Fill the array with random values in range [lower, upper)
    // Parallelized for NUMA first-touch: each thread initializes contiguous row chunks
    // Removed collapse(2) to ensure better NUMA locality for row-wise access patterns
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            unsigned long s = seed * concatenate(i, j);
            heatmap[i * cols + j] = my_rand(&s, lower, upper);
        }
    }
    
    return heatmap;
}

// Pre-process heatmap by applying hash function work_factor times
void preprocess_heatmap(unsigned long *heatmap, int rows, int cols, int work_factor) {
    // Apply hash function work_factor times to each element
    #pragma omp parallel for collapse(2) schedule(static)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            unsigned long val = heatmap[i * cols + j];
            for (int w = 0; w < work_factor; w++) {
                val = hash(val);
            }
            heatmap[i * cols + j] = val;
        }
    }
}

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc != 10) {
        fprintf(stderr, "Usage: %s <columns> <rows> <seed> <lower> <upper> <window_height> <verbose> <num_threads> <work_factor>\n", argv[0]);
        return 1;
    }
    
    
    // Parse command-line arguments
    int cols = atoi(argv[1]);
    int rows = atoi(argv[2]);
    unsigned long seed = strtoul(argv[3], NULL, 10);  
    unsigned long lower = strtoul(argv[4], NULL, 10); 
    unsigned long upper = strtoul(argv[5], NULL, 10); 
    int window_height = atoi(argv[6]); 
    int verbose = atoi(argv[7]);    
    int num_threads = atoi(argv[8]); 
    int work_factor = atoi(argv[9]);
    
    // Set number of OpenMP threads
    omp_set_num_threads(num_threads);
    
    // Validate input
    if (rows <= 0 || cols <= 0 || window_height <= 0 || window_height > rows || upper <= lower) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return 1;
    }
    
    // Print startup message and parameters
    printf("Starting heatmap_analysis\n");
    printf("Parameters: columns=%d, rows=%d, seed=%lu, lower=%lu, upper=%lu, window_height=%d, verbose=%d, num_threads=%d, work_factor=%d\n\n",
           cols, rows, seed, lower, upper, window_height, verbose, num_threads, work_factor);
    
    // Start timing immediately after reading command-line parameters
    double start_time = omp_get_wtime();
    
    // Step 1: Initialize heatmap
    unsigned long *heatmap = initialize_heatmap(rows, cols, seed, lower, upper);
    
    // Print original array if verbose (before transformation)
    if (verbose) {
        printf("A:\n");
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (j > 0) printf(", ");
                printf("%lu", heatmap[i * cols + j]);
            }
            printf("\n");
        }
    }
    
    // Step 2: Pre-process heatmap
    preprocess_heatmap(heatmap, rows, cols, work_factor);
    
    // Step 3 & 4: Combined parallel region for both Part A and Part B
    // Maximizes parallel region length to reduce thread creation/termination overhead
    unsigned long long *max_sums = (unsigned long long*) malloc(cols * sizeof(unsigned long long));
    padded_int *hotspots_per_row = (padded_int*) calloc(rows, sizeof(padded_int));
    int total_hotspots = 0;
    
    #pragma omp parallel
    {
        // Part A: Calculate maximum range sums for each column
        #pragma omp for schedule(static) nowait
        for (int col = 0; col < cols; col++) {
            unsigned long long max_sum = 0;
            unsigned long long current_sum = 0;
            
            // Calculate initial window sum
            for (int row = 0; row < window_height; row++) {
                current_sum += heatmap[row * cols + col];
            }
            max_sum = current_sum;
            
            // Slide the window down
            for (int row = window_height; row < rows; row++) {
                current_sum = current_sum - heatmap[(row - window_height) * cols + col] + heatmap[row * cols + col];
                if (current_sum > max_sum) {
                    max_sum = current_sum;
                }
            }
            
            max_sums[col] = max_sum;
        }
        
        // Part B: Count local hotspots
        // Reduction applied at the for directive level for clarity and correctness
        #pragma omp for schedule(static) reduction(+:total_hotspots)
        for (int i = 0; i < rows; i++) {
            int row_hotspots = 0;
            for (int j = 0; j < cols; j++) {
                unsigned long current = heatmap[i * cols + j];
                int is_hotspot = 1;
                
                // Check all 4 neighbors (up, down, left, right)
                // Check up (only if not first row)
                if (is_hotspot && i > 0 && heatmap[(i-1) * cols + j] >= current) {
                    is_hotspot = 0;
                }
                // Check down (only if not last row)
                if (is_hotspot && i < rows - 1 && heatmap[(i+1) * cols + j] >= current) {
                    is_hotspot = 0;
                }
                // Check left (only if not first column)
                if (is_hotspot && j > 0 && heatmap[i * cols + (j-1)] >= current) {
                    is_hotspot = 0;
                }
                // Check right (only if not last column)
                if (is_hotspot && j < cols - 1 && heatmap[i * cols + (j+1)] >= current) {
                    is_hotspot = 0;
                }
                
                if (is_hotspot) {
                    row_hotspots++;
                }
            }
            hotspots_per_row[i].count = row_hotspots;
            total_hotspots += row_hotspots;
        }
    }
    
    // Output results
    if (verbose) {
        // Print maximum sliding sums per column
        printf("\nMax sliding sums per column:\n");
        for (int col = 0; col < cols; col++) {
            if (col > 0) printf(", ");
            printf("%llu", max_sums[col]);
        }
        printf("\n\n");
        
        // Print hotspots per row
        printf("Hotspots per row:\n");
        for (int row = 0; row < rows; row++) {
            printf("Row %d: %d hotspot(s)\n", row, hotspots_per_row[row].count);
        }
        printf("\n");
    }
    
    printf("Total hotspots found: %d\n", total_hotspots);
    
    // End timing immediately after output (as per speedup measurement spec)
    double end_time = omp_get_wtime();
    double elapsed_time = end_time - start_time;
    printf("Execution took %.4f s\n", elapsed_time);
    
    // Clean up
    free(max_sums);
    free(hotspots_per_row);
    free(heatmap);
    
    return 0;
}