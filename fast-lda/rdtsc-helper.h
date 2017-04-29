#include "rdtsc.h"

typedef struct timer{
	tsc_counter start, end;
    double cycles;
    int num_runs;
    int flops;
} timer;

timer start_timer();
void stop_timer(timer t);

