#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../fast-lda/rdtsc-helper.h"

fp_t log_sum(fp_t log_a, fp_t log_b);
fp_t trigamma(fp_t x);
fp_t digamma(fp_t x);
fp_t log_gamma(fp_t x);
void make_directory(char* name);
int argmax(fp_t* x, int n);

#endif
