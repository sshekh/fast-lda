#ifndef LDA_INFERENCE_H
#define LDA_INFERENCE_H

#include <math.h>
#include <float.h>
#include <assert.h>
#include </opt/intel/compilers_and_libraries/mac/mkl/include/mkl.h>
#include "lda.h"
#include "utils.h"
#include "rdtsc-helper.h"


float VAR_CONVERGED;
int VAR_MAX_ITER;

/*
 * Variational inference
 * The optimizing values of the gamma and phi variational parameters
 * are found by minimizing KL divergence between the variational
 * distribution and the true posterior p(theta, z | w, alpha, beta).
 *
 */
fp_t lda_inference(document*, lda_model*, fp_t*, fp_t*);

/*
 * Compute the log likelihood bound with respect to the variational
 * parameters gamma and phi.
 */
fp_t compute_likelihood(document*, lda_model*, fp_t*, fp_t*);

#endif
