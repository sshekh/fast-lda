#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>

#include "rdtsc-helper.h"

fp_t log_sum(fp_t log_a, fp_t log_b);
fp_t trigamma(fp_t x);
fp_t digamma(fp_t x);
__m256fp digamma_vec(__m256fp x);
fp_t log_gamma(fp_t x);
int argmax(fp_t* x, int n);

#endif
