#include "rdtsc-helper.h"
#include "stdio.h"

timer start_timer(int id){
	timer t;
	t.id = id;
	CPUID(); RDTSC(t.start);
	return t;
}

void stop_timer(timer t){
	CPUID(); RDTSC(t.end);
    t.cycles = (long long) ((COUNTER_DIFF(t.end, t.start))/t.num_runs);
    timing_infrastructure[t.id].sum += t.cycles;
    timing_infrastructure[t.id].counter += 1;
    // printf("Run EM - Runtime [cycles]: %f\n", t.cycles);
    // printf("Run EM - Performance [flops/cycle]: %f\n", t.flops/t.cycles);
}

void init_timing_infrastructure(){
	for (int i=0;i<N_ACCUMULATORS;i++){
		timing_infrastructure[i].sum = 0;
		timing_infrastructure[i].counter = 0;
	}
}

void print_timings(){
	for (int i=0;i<N_ACCUMULATORS;i++){
		printf("%s - Total Runtime sum[cycles]: %lld\n", timer_names[i], timing_infrastructure[i].sum);
		if (timing_infrastructure[i].counter == 0){
			printf("ERROR. Denominaotr zero\n");
		}else {
			printf("%s - Runtime [cycles]: %lld\n", timer_names[i], timing_infrastructure[i].sum / timing_infrastructure[i].counter);
		}
	}
}
