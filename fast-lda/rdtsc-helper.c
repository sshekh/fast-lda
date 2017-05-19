#include "rdtsc-helper.h"


static accumulator timing_infrastructure[N_ACCUMULATORS];
static char* accumulator_names[] = {
    "RUN_EM",
    "LDA_INFERENCE",
    "DIGAMMA",
    "LOG_SUM",
    "LOG_GAMMA",
    "TRIGAMMA",
    "DOC_E_STEP",
    "LIKELIHOOD",
    "MLE",
    "OPT_ALPHA",
    "EM_CONVERGE",
    "INFERENCE_CONVERGE",
    "ALPHA_CONVERGE"};

timer start_timer(int id){
    timer t;
    t.id = id;

    // This CPUID thing seems to do nothing, but is required so that the CPU
    // doesn't schedule the RDTSC out-of-order.
    CPUID();
    RDTSC(t.start);
    return t;
}

void stop_timer(timer t){

    // This CPUID thing seems to do nothing, but is required so that the CPU
    // doesn't schedule the RDTSC out-of-order.
    CPUID();
    RDTSC(t.end);
    t.cycles = (long long) ((COUNTER_DIFF(t.end, t.start)));
    timing_infrastructure[t.id].sum += t.cycles;
    timing_infrastructure[t.id].counter += 1;
    // printf("Run EM - Runtime [cycles]: %f\n", t.cycles);
    // printf("Run EM - Performance [flops/cycle]: %f\n", t.flops/t.cycles);
}

void timer_manual_increment(int id, long long amount) {
    timing_infrastructure[id].sum += amount;
    timing_infrastructure[id].counter++;
}

void init_timing_infrastructure() {
    for (int i=0;i<N_ACCUMULATORS;i++){
        timing_infrastructure[i].sum = 0;
        timing_infrastructure[i].counter = 0;
    }
}

void print_timings(FILE* f) {
    fprintf(f, "Accumulator, Total count, Average count\n");
    for (int i=0;i<N_ACCUMULATORS;i++){
        if (timing_infrastructure[i].counter == 0){
            fprintf(f, "%s, The code inside this timer has not been called, 0\n", accumulator_names[i]);
        }else {
            fprintf(f, "%s, %lld, %f\n",
                accumulator_names[i],
                timing_infrastructure[i].sum,
                (double) timing_infrastructure[i].sum / (double) timing_infrastructure[i].counter);
        }
    }
}
