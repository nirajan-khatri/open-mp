#!/usr/bin/env bash
####### Job Name #######
#SBATCH --job-name="pi_tasks_speedup"

####### Partition #######
#SBATCH --partition=all

####### Resources #######
#SBATCH --time=0-02:00:00

####### Node Info #######
#SBATCH --exclusive
#SBATCH --nodes=1

####### Output #######
#SBATCH --output=./out/pi_tasks_speedup.out.%j
#SBATCH --error=./out/pi_tasks_speedup.err.%j

# Load required modules (if needed on Fulda HPC)
# module load gcc/14.3.0

# Test parameters as specified in the assignment
NUM_TASKS=10000
LOWER=10000
UPPER=1000000
SEED=42

echo ""
echo "Pi Tasks Speedup Measurement"
echo ""
echo "Parameters: num_tasks=$NUM_TASKS, lower=$LOWER, upper=$UPPER, seed=$SEED"
echo "Node: $(hostname)"
echo "Date: $(date)"
echo ""

# Array to store execution times
declare -a times

# Test with different thread counts
for THREADS in 1 2 4 8 16 32 64
do
    echo ""
    echo "Running with $THREADS thread(s)..."
    echo ""
    
    # Run the program and capture output
    output=$(./pi_tasks $NUM_TASKS $THREADS $LOWER $UPPER $SEED)
    
    # Display output
    echo "$output"
    
    # Extract execution time (format: "Execution took X.XXXX s")
    time=$(echo "$output" | grep "Execution took" | awk '{print $3}')
    times[$THREADS]=$time
    
    echo ""
done

echo ""
echo "Speedup Measurement Summary"
echo ""
echo "Thread Count | Time (s) | Speedup | Efficiency"
echo "-------------|----------|---------|------------"

# Calculate and display speedups
baseline=${times[1]}
for THREADS in 1 2 4 8 16 32 64
do
    time=${times[$THREADS]}
    if [ -n "$time" ] && [ -n "$baseline" ]; then
        speedup=$(awk -v b="$baseline" -v t="$time" 'BEGIN {printf "%.4f", b/t}')
        efficiency=$(awk -v s="$speedup" -v n="$THREADS" 'BEGIN {printf "%.4f", s/n*100}')
        printf "%12d | %8s | %7s | %9s%%\n" $THREADS $time $speedup $efficiency
    fi
done

echo ""
echo "Speedup measurement complete!"
