#!/bin/bash
#SBATCH --job-name=heatmap_speedup
#SBATCH --output=/home/fd-%u/out/heatmap_speedup_%j.out
#SBATCH --error=/home/fd-%u/out/heatmap_speedup_%j.err
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64

# Load required modules
module load gcc/14.3.0

# Test parameters (adjust for good runtime - aim for 10-60 seconds sequential)
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
echo ""

# Test with different thread counts
for THREADS in 1 2 4 8 16 32 64
do
    echo "----------------------------------------"
    echo "Running with $THREADS thread(s)..."
    echo "----------------------------------------"
    export OMP_NUM_THREADS=$THREADS
    ./heatmap_analysis $COLS $ROWS $SEED $LOWER $UPPER $WINDOW_HEIGHT $VERBOSE $THREADS $WORK_FACTOR
    echo ""
done

echo "======================================"
echo "Speedup measurement complete!"
echo "======================================"
