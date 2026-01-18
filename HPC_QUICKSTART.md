# Quick Start Guide for Fulda HPC Cluster

## Step 1: Prepare Files Locally

Make sure you have:

- `heatmap_analysis.c` ✓
- `Makefile` ✓
- `run_heatmap_speedup.sh` ✓

## Step 2: Upload to HPC

```bash
# Replace fd-0003916 with your actual FD number
scp heatmap_analysis.c fd-0003916@hpc.informatik.hs-fulda.de:~/
scp Makefile fd-0003916@hpc.informatik.hs-fulda.de:~/
scp run_heatmap_speedup.sh fd-0003916@hpc.informatik.hs-fulda.de:~/
```

## Step 3: Login to HPC

```bash
ssh fd-0003916@hpc.informatik.hs-fulda.de
```

## Step 4: Setup on HPC

```bash
# Create output directory
mkdir -p ~/out

# Load compiler
module load gcc/14.3.0

# Compile
gcc -fopenmp -O3 -Wall -o heatmap_analysis heatmap_analysis.c -lm

# Quick test (optional)
./heatmap_analysis 3 4 42 0 10 2 1 1 1
```

## Step 5: Submit Job

```bash
# Make script executable
chmod +x run_heatmap_speedup.sh

# Submit to queue
sbatch run_heatmap_speedup.sh

# Check status
squeue -u fd-0003916
```

## Step 6: Get Results

```bash
# After job completes, view output
cat ~/out/heatmap_speedup_*.out

# Download results to your PC
# (run from your local machine)
scp fd-0003916@hpc.informatik.hs-fulda.de:~/out/heatmap_speedup_*.out ./
```

## Troubleshooting

**Connection timeout?**

- Make sure you're on campus network or connected via VPN

**Job not starting?**

- Check queue: `squeue`
- Check your jobs: `squeue -u fd-0003916`

**Need to cancel?**

```bash
scancel <JOBID>
# or cancel all your jobs
scancel -u fd-0003916
```

**Adjust runtime:**
Edit `run_heatmap_speedup.sh` and change:

- `COLS=2048` (increase for longer runtime)
- `ROWS=2048` (increase for longer runtime)
- `WORK_FACTOR=50` (increase for more computation)

Aim for sequential runtime of 10-60 seconds for good measurements.
