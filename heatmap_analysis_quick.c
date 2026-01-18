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

// Initialize heatmap with random values
unsigned long* initialize_heatmap(int rows, int cols, int seed, int lower, int upper) {
    // Allocate flat 1D array to represent 2D matrix (unsigned long as per spec)
    unsigned long *heatmap = (unsigned long*) malloc(rows * cols * sizeof(unsigned long));
    
    if (heatmap == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(1);
    }
    
    // Fill the array with random values in range [lower, upper)
    // Each cell gets a unique seed based on its position
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
    for (int iteration = 0; iteration < work_factor; iteration++) {
        #pragma omp parallel for collapse(2)
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                heatmap[i * cols + j] = hash(heatmap[i * cols + j]);
            }
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
    int cols = atoi(argv[1]);       // columns
    int rows = atoi(argv[2]);       // rows
    int seed = atoi(argv[3]);       // seed
    int lower = atoi(argv[4]);      // lower bound
    int upper = atoi(argv[5]);      // upper bound
    int window_height = atoi(argv[6]); // window_height
    int verbose = atoi(argv[7]);    // verbose
    int num_threads = atoi(argv[8]); // num_threads
    int work_factor = atoi(argv[9]); // work_factor
    
    // Set number of OpenMP threads
    omp_set_num_threads(num_threads);
    
    // Validate input
    if (rows <= 0 || cols <= 0 || window_height <= 0 || window_height > rows || upper <= lower) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return 1;
    }
    
    // Print startup message and parameters
    printf("Starting heatmap_analysis\n");
    printf("Parameters: columns=%d, rows=%d, seed=%d, lower=%d, upper=%d, window_height=%d, verbose=%d, num_threads=%d, work_factor=%d\n",
           cols, rows, seed, lower, upper, window_height, verbose, num_threads, work_factor);
    
    // Start timing
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
    
    // Step 3: Part A - Calculate maximum range sums for each column
    unsigned long long *max_sums = (unsigned long long*) malloc(cols * sizeof(unsigned long long));
    
    #pragma omp parallel for
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
    
    // Step 4: Part B - Count local hotspots with early termination
    // Use padded structure to prevent false sharing between threads
    padded_int *hotspots_per_row = (padded_int*) calloc(rows, sizeof(padded_int));
    int total_hotspots = 0;
    int early_exit_row = -1;  // Track which row triggered early exit
    int should_exit = 0;       // Shared flag for early termination
    
    #pragma omp parallel for reduction(+:total_hotspots) schedule(dynamic)
    for (int i = 0; i < rows; i++) {
        // Check if another thread already found a row with zero hotspots
        if (should_exit) {
            continue;  // Skip remaining rows
        }
        
        int row_hotspots = 0;
        for (int j = 0; j < cols; j++) {
            unsigned long current = heatmap[i * cols + j];
            int is_hotspot = 1;
            
            // Check all 4 neighbors (up, down, left, right)
            // Only check neighbors that exist - edge cells have fewer neighbors
            
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
        
        // Check for early exit condition
        if (row_hotspots == 0) {
            #pragma omp critical
            {
                if (!should_exit) {
                    should_exit = 1;
                    early_exit_row = i;
                }
            }
        }
    }
    
    // End timing
    double end_time = omp_get_wtime();
    double elapsed_time = end_time - start_time;
    
    // Check if early exit occurred
    if (early_exit_row != -1) {
        printf("Row %d contains no hotspots.\n", early_exit_row);
        printf("Early exit.\n");
        printf("Execution took %.4f s\n", elapsed_time);
    } else {
        // Normal output - all rows have at least one hotspot
        if (verbose) {
            // Print maximum sliding sums per column
            printf("Max sliding sums per column:\n");
            for (int col = 0; col < cols; col++) {
                if (col > 0) printf(",");
                printf("%llu", max_sums[col]);
            }
            printf("\n");
            
            // Print hotspots per row
            printf("Hotspots per row:\n");
            for (int row = 0; row < rows; row++) {
                printf("Row %d: %d hotspot(s)\n", row, hotspots_per_row[row].count);
            }
        }
        
        printf("Total hotspots found: %d\n", total_hotspots);
        printf("Execution took %.4f s\n", elapsed_time);
    }
    
    // Clean up
    free(max_sums);
    free(hotspots_per_row);
    
    // Clean up
    free(heatmap);
    
    return 0;
}
