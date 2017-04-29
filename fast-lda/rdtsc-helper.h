#ifndef RDTSC_HELPER
#define RDTSC_HELPER

#include "rdtsc.h"

typedef struct timer{
	int id;
	tsc_counter start, end;
    long long cycles;
} timer;

timer start_timer(int id);
void stop_timer(timer t);

// Always adapt both enum and string array!
enum timer_ids{RUN_EM, LDA_INFERENCE, DIGAMMA, LOG_SUM, LOG_GAMMA, DOC_E_STEP, LIKELIHOOD, N_ACCUMULATORS};
extern char* timer_names[N_ACCUMULATORS];


typedef struct {
	long long sum;
	long counter;
} accumulator;

accumulator timing_infrastructure[N_ACCUMULATORS];

void init_timing_infrastructure();
void print_timings();

#endif


