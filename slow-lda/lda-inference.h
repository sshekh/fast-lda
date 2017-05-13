#ifndef LDA_INFERENCE_H
#define LDA_INFERENCE_H

#include <math.h>
#include <float.h>
#include <assert.h>
#include "lda.h"
#include "utils.h"

float VAR_CONVERGED;
int VAR_MAX_ITER;

fp_t lda_inference(document*, lda_model*, fp_t*, fp_t**);
fp_t compute_likelihood(document*, lda_model*, fp_t**, fp_t*);

#endif
