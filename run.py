#! /usr/bin/python

################################################################################

# Script to run the algorithm for different input sizes to record timings.
# The results are saved to the folder 
#	./profiling
# and the results are named
#   slow/fast_timings<NDOCS>.csv

# Adjust the document collection size and step with START, END, STEP.

################################################################################

import subprocess

START = 2
END = 20
STEP = 10

# Fast
for NDOCS in range(START, END, STEP):
	subprocess.check_call(["./fast-lda/lda", "est", str(NDOCS), "1", "50", "./fast-lda/settings.txt", "./fast-lda/ap/ap.dat", "seeded", "logfiles"])
	source = "./results/fast_timings.csv"
	destination = "./profiling/fast_timings" + str(NDOCS) + ".csv"
	subprocess.call(["mv", source, destination])

# Slow 
# for NDOCS in range(2, 20, 10):
# 	subprocess.check_call(["./slow-lda/lda", "est", str(NDOCS), "1", "50", "./slow-lda/settings.txt", "./slow-lda/ap/ap.dat", "seeded", "logfiles"])
# 	source = "./results/slow_timings.csv"
# 	destination = "./profiling/slow_timings" + str(NDOCS) + ".csv"
# 	subprocess.call(["mv", source, destination])	