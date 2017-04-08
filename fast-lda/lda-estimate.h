#ifndef LDA_ESTIMATE_H
#define LDA_ESTIMATE_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <string.h>

#include "lda.h"
#include "lda-inference.h"
#include "lda-model.h"
#include "lda-alpha.h"
#include "utils.h"
#include "cokus.h"

/*
 * The LDA parameter estimation functionality. 
 */

float EM_CONVERGED;
int EM_MAX_ITER;
int ESTIMATE_ALPHA;
double INITIAL_ALPHA;
int NTOPICS;

/*
 * Perform inference on a document and update sufficient statistics.
 * The E-Step maximizes the bound of the log likelihood with respect
 * to the variational parameters gamma and phi. Performing the updates
 * in equations (16) and (17).
 */
double doc_e_step(document* doc,
                  double* gamma,
                  double** phi,
                  lda_model* model,
                  lda_suffstats* ss);

/*
 * The EM algorithm with the following steps:
 *      E-Step: For each document d, find the optimal variational
                parameters gamma_d * and phi_d *.
 *              This step is performed by the doc_e_step function.
 *
 *      M-Step: Maximize the resulting lower bound on the log likelihood
 *              with respect to the model params: alpha and beta.
 *              For this we maximize the likelihood estimate for each document
 *              using the approximate posterior computed in the E-Step.
 *              Coordinate ascent on the log likelihood.
 */

void run_em(char* start,
            char* directory,
            corpus* corpus);

#endif


