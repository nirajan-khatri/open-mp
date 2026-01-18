# Makefile for OpenMP Exam Tasks
# Author: [Your Name]
# Compiler: gcc/14.3.0 (as required by specification)

CC = gcc
CFLAGS = -fopenmp -O3 -Wall -Wextra
LDFLAGS = -lm

# Targets
TARGETS = heatmap_analysis heatmap_analysis_quick pi_tasks

all: $(TARGETS)

heatmap_analysis: heatmap_analysis.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

heatmap_analysis_quick: heatmap_analysis_quick.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

pi_tasks: pi_tasks.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
