#include <stdlib.h>
#include <stdio.h>

#include "../framework/dict.h"
#include "../framework/vector.h"
#include "doc.h"
#include "topics.h"

// The maximum length of a document filename we support.
#define DOC_FNAME_LENGTH 256

doc_t* load_doc(FILE* fhdl, dict_t* dict) {
    // TODO
    // Open the file and read its contents
    // Tokenize, potentially remove stop words
    // Add the words to the dictionary and to doc->words
    // Randomly initialize doc->mixture and doc->word_topics
    // The NUM_TOPICS global provides the appropriate sizes.

    return NULL;
}

corpus_t load_docs(const char* corpus_file) {
    FILE* corpus_fhdl = fopen(corpus_file, "r");
    if (!corpus_fhdl) {
        fprintf(stderr, "Cannot open corpus file %s.\n", corpus_file);
        exit(1);
    }

    char doc_fname[DOC_FNAME_LENGTH] = {0};

    doc_t** docs_vector = vec_alloc(100, sizeof(doc_t*));
    dict_t* dict = dict_alloc();

    while(!feof(corpus_fhdl)) {
        fgets(doc_fname, DOC_FNAME_LENGTH, corpus_fhdl);

        // fgets copies the newline character if it's present.
        if (doc_fname[DOC_FNAME_LENGTH - 1] == '\n') {
            doc_fname[DOC_FNAME_LENGTH - 1] = '\0';
        }

        FILE* this_doc = fopen(doc_fname, "r");
        if (!this_doc) {
            fprintf(stderr, "Warning: cannot open %s. Skipping...\n", doc_fname);
            continue;
        }

        doc_t* doc_p = load_doc(this_doc, dict);
        docs_vector = vec_append(&docs_vector, &doc_p, sizeof(doc_t*));
        fclose(this_doc);
    }

    fclose(corpus_fhdl);

    corpus_t corp = { docs_vector, dict };
    return corp;
}
