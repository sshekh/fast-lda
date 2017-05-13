#ifndef LDA_MODEL_H
#define LDA_MODEL_H

/*
 * This is the functionality for LDA model construction and MLE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "lda.h"
#include "lda-alpha.h"
#include "cokus.h"

#define myrand() (fp_t) (((unsigned long) randomMT()) / 4294967296.)
#define NUM_INIT 1

/*
 * compute MLE lda model from sufficient statistics
 *
 */
void lda_mle(lda_model* model, lda_suffstats* ss, int estimate_alpha);


/*
 * Allocate new lda model.
 *
 */
lda_model* new_lda_model(int, int, int);
/*
 * Deallocate new lda model.
 *
 */
void free_lda_model(lda_model*);
/*
 * Load and already computed lda model.
 *
 */
lda_model* load_lda_model(char* model_root);
/*
 * Save current state of lda model.
 *
 */
void save_lda_model(lda_model*, char*, int);


/*
 * Allocate sufficient statistics
 *
 */
lda_suffstats* new_lda_suffstats(lda_model* model);

/*
 * Initialize sufficient statistics randomly.
 *
 */
void random_initialize_ss(lda_suffstats* ss, lda_model* model);
/*
 * Initialize sufficient statistics to zero values.
 *
 */
void zero_initialize_ss(lda_suffstats* ss, lda_model* model);


#endif
