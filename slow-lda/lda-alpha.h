#ifndef LDA_ALPHA_H
#define LDA_ALPHA_H

#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "lda.h"
#include "utils.h"

#define NEWTON_THRESH 1e-5
#define MAX_ALPHA_ITER 1000

fp_t alhood(fp_t a, fp_t ss, int D, int K);
fp_t d_alhood(fp_t a, fp_t ss, int D, int K);
fp_t d2_alhood(fp_t a, int D, int K);
fp_t opt_alpha(fp_t ss, int D, int K);
void maximize_alpha(fp_t** gamma, lda_model* model, int num_docs);

#endif
