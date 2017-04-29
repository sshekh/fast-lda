#include "rdtsc.h"

#define N_ACCUMULATORS 7

typedef struct timer{
	int id;
	tsc_counter start, end;
    long long cycles;
    int num_runs;
    int flops;
} timer;

timer start_timer(int id);
void stop_timer(timer t);

// Always adapt both enum and string array!
static const enum timer_ids{RUN_EM, LDA_INFERENCE, DIGAMMA, LOG_SUM, LOG_GAMMA, DOC_E_STEP, LIKELIHOOD};
static const char* timer_names[] = {"RUN_EM", "LDA_INFERENCE", "DIGAMMA", "LOG_SUM", "LOG_GAMMA", "DOC_E_STEP", "LIKELIHOOD"};


typedef struct {
	long long sum;
	long counter;
} accumulator;

accumulator timing_infrastructure[N_ACCUMULATORS];

void init_timing_infrastructure();
void print_timings();



