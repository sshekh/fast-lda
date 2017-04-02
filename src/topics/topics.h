#ifndef TOPICS_H
#define TOPICS_H

#include "doc.h"

// Global
extern int NUM_TOPICS;

/* Given corpus-wide parameters alpha (corpus topic mixture) and beta
 * (word-topic affinities), infer the document topic mixture and word-topic
 * associations. doc is modified in-place.
 */
void infer(doc_t* doc, const float* alpha, const float* beta);

/* Given a corpus with document-topic mixtures and word-topic associations,
 * learn the corpus-wide parameters (corpus topic mixture and word-topic affinities).
 */
void learn(const corpus_t corpus, float* alpha, float* beta);

#endif