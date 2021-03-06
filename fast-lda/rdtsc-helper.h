#ifndef RDTSC_HELPER
#define RDTSC_HELPER

// Note: always ensure that stdio is included before we do this define printf
// macro. Otherwise we end up replacing it in stdio, leading to compiler errors.
#include <stdio.h>

#ifdef IGNORE_PRINTF
    #define printf(fmt, ...)
#endif

#include "rdtsc.h"

typedef struct timer{
    int id;
    tsc_counter start, end;
    long long cycles;
} timer;

timer start_timer(int id);
void stop_timer(timer t);

// Always adapt both enum and string array (in rdtsc_helper.c)!
enum accumulator_ids{
    RUN_EM,
    LDA_INFERENCE,
    DIGAMMA,
    LOG_SUM,
    LOG_GAMMA,
    TRIGAMMA,
    DOC_E_STEP,
    LIKELIHOOD,
    MLE,
    OPT_ALPHA,
    EM_CONVERGE,
    INFERENCE_CONVERGE,
    ALPHA_CONVERGE,
    N_ACCUMULATORS};

typedef struct {
    long long sum;
    long counter;
} accumulator;

void init_timing_infrastructure();
void print_timings();

// Gives direct access to a counter (basically only used for the converge counters)
void timer_manual_increment(int id, long long amount);

#endif


