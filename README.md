# OpenMP Examination - Parallel Programming

**Student:** [Your Name]  
**Team Partner:** [Partner Name if applicable]

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

## Files Included

- `heatmap_analysis.c` - Task 1.1 implementation
- `heatmap_analysis_quick.c` - Task 1.2 implementation (to be completed)
- `pi_tasks.c` - Task 1.3 implementation (to be completed)
- `Makefile` - Build configuration
- `run_heatmap_speedup.sh` - Slurm script for Task 1.1 benchmarks
- `run_heatmap_quick_speedup.sh` - Slurm script for Task 1.2 benchmarks
- `run_pi_speedup.sh` - Slurm script for Task 1.3 benchmarks
- `*.out` - Slurm output files with timing results
- `speedup_report.pdf` - Speedup analysis with plots and tables

## Notes

- All programs use the exact helper functions (my_rand, hash, concatenate) from the specification
- Code has been tested locally and should produce correct results on HPC cluster
- Timing measurements start after parameter parsing and end after output
- Thread counts tested: 1, 2, 4, 8, 16, 32, 64

## Authorship

- **Task 1.1:** [Author]
- **Task 1.2:** [Author]
- **Task 1.3:** [Author]
- **Helper functions:** From specification (Page 7)
