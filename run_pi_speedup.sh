#!/bin/bash
#SBATCH --job-name=pi_tasks_speedup
#SBATCH --output=/home/fd-%u/out/pi_tasks_speedup_%j.out
#SBATCH --error=/home/fd-%u/out/pi_tasks_speedup_%j.err
#SBATCH --time=02:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64

# Load required modules
module load gcc/14.3.0

# Test parameters as specified in the assignment
NUM_TASKS=20000
LOWER=10000
UPPER=1000000
SEED=42

echo "======================================"
echo "Pi Tasks Speedup Measurement"
echo "======================================"
echo "Parameters: num_tasks=$NUM_TASKS, lower=$LOWER, upper=$UPPER, seed=$SEED"
echo ""

# Test with different thread counts
for THREADS in 1 2 4 8 16 32 64
do
    echo "----------------------------------------"
    echo "Running with $THREADS thread(s)..."
    echo "----------------------------------------"
    export OMP_NUM_THREADS=$THREADS
    ./pi_tasks $NUM_TASKS $THREADS $LOWER $UPPER $SEED
    echo ""
done

echo "======================================"
echo "Speedup measurement complete!"
echo "======================================"
