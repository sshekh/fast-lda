#ifndef LDA_ESTIMATE_HELPER_H
#define LDA_ESTIMATE_HELPER_H

/*
 * Helper functions for LDA estimation.
 */

#include "lda.h"
#include <stdio.h>
#include <string.h>

/*
 * Saves the gamma parameters of the current dataset
 *
 */
void save_gamma(char* filename, fp_t** gamma, int num_docs, int num_topics);

/*
 * Writes the word assignments line for a document to a file
 *
 */
void write_word_assignment(FILE* f, document* doc, fp_t* phi, lda_model* model);

/*
 * Computes teh maximum length of a document in the corpus.
 *
 */
int max_corpus_length(corpus* c);

/*
 * Gather all rows from matrices used per document.
 *
 */
void gatherDocWords(fp_t* entire_matrix, fp_t* reduced_matrix,
					document* doc, size_t row_size);

/*
 * Scatter all rows from matrix used per document to the original large matrix.
 *
 */
void scatterDocWords(fp_t* entire_matrix, fp_t* reduced_matrix,
					 document* doc, size_t row_size);



#endif