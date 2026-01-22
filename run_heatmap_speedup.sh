#!/usr/bin/env bash
####### Job Name #######
#SBATCH --job-name="heatmap_speedup"

####### Partition #######
#SBATCH --partition=all

####### Resources #######
#SBATCH --time=0-01:00:00

####### Node Info #######
#SBATCH --exclusive
#SBATCH --nodes=1

####### Output #######
#SBATCH --output=./out/heatmap_speedup.out.%j
#SBATCH --error=./out/heatmap_speedup.err.%j

# Load required modules (if needed on Fulda HPC)
# module load gcc
# module load openmpi

# Test parameters (adjusted for 10-60 seconds sequential runtime)
COLS=2048
ROWS=2048
SEED=42
LOWER=0
UPPER=100
WINDOW_HEIGHT=50
VERBOSE=0
WORK_FACTOR=50

echo "======================================"
echo "Heatmap Analysis Speedup Measurement"
echo "======================================"
echo "Parameters: columns=$COLS, rows=$ROWS, seed=$SEED"
echo "lower=$LOWER, upper=$UPPER, window_height=$WINDOW_HEIGHT"
echo "work_factor=$WORK_FACTOR"
echo "Node: $(hostname)"
echo "Date: $(date)"
echo ""

# Array to store execution times
declare -a times

# Test with different thread counts
for THREADS in 1 2 4 8 16 32 64
do
    echo "----------------------------------------"
    echo "Running with $THREADS thread(s)..."
    echo "----------------------------------------"
    
    # Run the program and capture output
    output=$(./heatmap_analysis $COLS $ROWS $SEED $LOWER $UPPER $WINDOW_HEIGHT $VERBOSE $THREADS $WORK_FACTOR)
    
    # Display output
    echo "$output"
    
    # Extract execution time (format: "Execution took X.XXXX s")
    time=$(echo "$output" | grep "Execution took" | awk '{print $3}')
    times[$THREADS]=$time
    
    echo ""
done

echo "======================================"
echo "Speedup Measurement Summary"
echo "======================================"
echo ""
echo "Thread Count | Time (s) | Speedup | Efficiency"
echo "-------------|----------|---------|------------"

# Calculate and display speedups
baseline=${times[1]}
for THREADS in 1 2 4 8 16 32 64
do
    time=${times[$THREADS]}
    if [ -n "$time" ] && [ -n "$baseline" ]; then
        speedup=$(echo "scale=4; $baseline / $time" | bc)
        efficiency=$(echo "scale=4; $speedup / $THREADS * 100" | bc)
        printf "%12d | %8s | %7s | %9s%%\n" $THREADS $time $speedup $efficiency
    fi
done

echo ""
echo "======================================"
echo "Speedup measurement complete!"
echo "======================================"