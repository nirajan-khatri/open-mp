#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

// Hash function from assignment PDF
int hash(int value) {
    return (value * 1103515245 + 12345) % (1 << 31);
}

// Random integer generator from assignment PDF
int random_int(int *seed, int max) {
    *seed = hash(*seed);
    return *seed % max;
}

// Initialize heatmap with random values
int* initialize_heatmap(int rows, int cols, int seed) {
    // Allocate flat 1D array to represent 2D matrix
    int *heatmap = (int*) malloc(rows * cols * sizeof(int));
    
    if (heatmap == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(1);
    }
    
    int current_seed = seed;
    
    // Fill the array with random values
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            heatmap[i * cols + j] = random_int(&current_seed, 1000);
        }
    }
    
    return heatmap;
}

// Pre-process heatmap by applying hash function work_factor times
void preprocess_heatmap(int *heatmap, int rows, int cols, int work_factor) {
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
    if (argc != 7) {
        fprintf(stderr, "Usage: %s <rows> <cols> <seed> <work_factor> <window_height> <verbose>\n", argv[0]);
        return 1;
    }
    
    // Parse command-line arguments
    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);
    int seed = atoi(argv[3]);
    int work_factor = atoi(argv[4]);
    int window_height = atoi(argv[5]);
    int verbose = atoi(argv[6]);
    
    // Validate input
    if (rows <= 0 || cols <= 0 || window_height <= 0 || window_height > rows) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return 1;
    }
    
    // Start timing
    double start_time = omp_get_wtime();
    
    // Step 1: Initialize heatmap
    int *heatmap = initialize_heatmap(rows, cols, seed);
    
    // Step 2: Pre-process heatmap
    preprocess_heatmap(heatmap, rows, cols, work_factor);
    
    // Step 3: Part A - Calculate maximum range sums for each column
    long long *max_sums = (long long*) malloc(cols * sizeof(long long));
    
    #pragma omp parallel for
    for (int col = 0; col < cols; col++) {
        long long max_sum = 0;
        long long current_sum = 0;
        
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
    
    // Step 4: Part B - Count local hotspots
    int *hotspots_per_row = (int*) calloc(rows, sizeof(int));
    int total_hotspots = 0;
    
    #pragma omp parallel for reduction(+:total_hotspots)
    for (int i = 0; i < rows; i++) {
        int row_hotspots = 0;
        for (int j = 0; j < cols; j++) {
            int current = heatmap[i * cols + j];
            int is_hotspot = 1;
            
            // Check all 4 neighbors (up, down, left, right)
            // If any neighbor doesn't exist or is >= current, not a hotspot
            
            // Check up
            if (i == 0 || heatmap[(i-1) * cols + j] >= current) {
                is_hotspot = 0;
            }
            // Check down
            if (is_hotspot && (i == rows - 1 || heatmap[(i+1) * cols + j] >= current)) {
                is_hotspot = 0;
            }
            // Check left
            if (is_hotspot && (j == 0 || heatmap[i * cols + (j-1)] >= current)) {
                is_hotspot = 0;
            }
            // Check right
            if (is_hotspot && (j == cols - 1 || heatmap[i * cols + (j+1)] >= current)) {
                is_hotspot = 0;
            }
            
            if (is_hotspot) {
                row_hotspots++;
            }
        }
        hotspots_per_row[i] = row_hotspots;
        total_hotspots += row_hotspots;
    }
    
    // End timing
    double end_time = omp_get_wtime();
    double elapsed_time = end_time - start_time;
    
    // Output results
    if (verbose) {
        // Print maximum sums for each column
        printf("Maximum range sums per column:\n");
        for (int col = 0; col < cols; col++) {
            printf("Column %d: %lld\n", col, max_sums[col]);
        }
        printf("\n");
        
        // Print hotspots per row
        printf("Local hotspots per row:\n");
        for (int row = 0; row < rows; row++) {
            printf("Row %d: %d hotspots\n", row, hotspots_per_row[row]);
        }
        printf("\n");
    }
    
    printf("Total local hotspots: %d\n", total_hotspots);
    printf("Execution time: %.6f seconds\n", elapsed_time);
    
    // Clean up
    free(max_sums);
    free(hotspots_per_row);
    
    // Clean up
    free(heatmap);
    
    return 0;
}
