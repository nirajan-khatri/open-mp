#!/usr/bin/env bash
####### Job Name #######
#SBATCH --job-name="heatmap_quick_speedup"

####### Partition #######
#SBATCH --partition=all

####### Resources #######
#SBATCH --time=0-01:00:00

####### Node Info #######
#SBATCH --exclusive
#SBATCH --nodes=1

####### Output #######
#SBATCH --output=./out/heatmap_quick_speedup.out.%j
#SBATCH --error=./out/heatmap_quick_speedup.err.%j

# Test parameters - should trigger early exit for interesting speedup comparison
COLS=2048
ROWS=2048
SEED=42
LOWER=0
UPPER=100
WINDOW_HEIGHT=50
VERBOSE=0
WORK_FACTOR=0  # No transformation - more likely to have early exits

echo "======================================"
echo "Heatmap Quick Analysis Speedup Measurement"
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
    ./heatmap_analysis_quick $COLS $ROWS $SEED $LOWER $UPPER $WINDOW_HEIGHT $VERBOSE $THREADS $WORK_FACTOR
    echo ""
done

echo "======================================"
echo "Speedup measurement complete!"
echo "======================================"
