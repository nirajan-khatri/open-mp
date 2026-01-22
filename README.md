# OpenMP Examination - Parallel Programming

**Student:** Nirajan Khatri

## Compilation Instructions

### On HPC Cluster:

```bash
# Load required compiler
module load gcc/14.3.0

# Compile all programs
make clean
make all
```

### Manual Compilation:

```bash
gcc -fopenmp -O3 -o heatmap_analysis heatmap_analysis.c -lm
gcc -fopenmp -O3 -o heatmap_analysis_quick heatmap_analysis_quick.c -lm
gcc -fopenmp -O3 -o pi_tasks pi_tasks.c -lm
```

## Execution Instructions

### Task 1.1: heatmap_analysis

```bash
./heatmap_analysis <columns> <rows> <seed> <lower> <upper> <window_height> <verbose> <num_threads> <work_factor>
```

**Example:**

```bash
./heatmap_analysis 3 4 42 0 10 2 1 1 1
```

**Speedup Measurement:**

```bash
# Submit Slurm job
sbatch run_heatmap_speedup.sh

# Check job status
squeue -u $USER

# View results
cat heatmap_analysis_<job_id>.out
```

### Task 1.2: heatmap_analysis_quick

```bash
./heatmap_analysis_quick <columns> <rows> <seed> <lower> <upper> <window_height> <verbose> <num_threads> <work_factor>
```

**Example:**

```bash
./heatmap_analysis_quick 3 4 42 0 10 2 1 1 0
```

**Speedup Measurement:**

```bash
sbatch run_heatmap_quick_speedup.sh
```

### Task 1.3: pi_tasks

```bash
./pi_tasks <num_tasks> <num_threads> <lower> <upper> <seed>
```

**Example:**

```bash
./pi_tasks 1000 4 100000 1000000 42
```

**Speedup Measurement:**

```bash
sbatch run_pi_speedup.sh
```
