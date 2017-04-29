#include "rdtsc-helper.h"
#include "stdio.h"

timer start_timer(){
	timer t;
	CPUID(); RDTSC(t.start);
	return t;
}

void stop_timer(timer t){
	CPUID(); RDTSC(t.end);

    t.cycles = (double)(COUNTER_DIFF(t.end, t.start))/t.num_runs;
    printf("Run EM - Runtime [cycles]: %f\n", t.cycles);
    // printf("Run EM - Performance [flops/cycle]: %f\n", t.flops/t.cycles);
}
