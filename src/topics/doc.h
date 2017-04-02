#ifndef DOC_H
#define DOC_H

#include "../framework/dict.h"

struct doc_t {

  // Number of words in this document.
  int len;

  // Actual words in this document, represented by integers; size [len]
  int* words;

  // Current assignments of topics to this document (gamma); size K
  float* mixture;

  // Current assignments of topics to the words (phi); size [len x K]
  float* word_topics;
};
typedef struct doc_t doc_t;


struct corpus_t {
    // A vector of pointers to the documents in this corpus
    doc_t** docs_v;

    // The dictionary of the corpus, recording all the words and theirs indices.
    dict_t* dictionary;
};

typedef struct corpus_t corpus_t;

/* Load a corpus given a file containing a list of paths to documents. */
corpus_t load_docs(const char* corpus_file);


#endif