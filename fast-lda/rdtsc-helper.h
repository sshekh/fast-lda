#ifndef RDTSC_HELPER
#define RDTSC_HELPER


// Disable all printfs
// #define IGNORE_PRINTF

#ifdef IGNORE_PRINTF
    #define printf(fmt, ...)  (void)
#endif

// #define DOUBLE
#ifdef DOUBLE
    #define fp_t double
#else
    #define fp_t float
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
enum accumulator_ids{RUN_EM, LDA_INFERENCE, DIGAMMA, LOG_SUM, LOG_GAMMA, DOC_E_STEP, LIKELIHOOD, EM_CONVERGE, INFERENCE_CONVERGE, N_ACCUMULATORS};
extern char* accumulator_names[N_ACCUMULATORS];


typedef struct {
    long long sum;
    long counter;
} accumulator;

extern accumulator timing_infrastructure[N_ACCUMULATORS];

void init_timing_infrastructure();
void print_timings();

#endif


