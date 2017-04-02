#include <stdio.h>
#include <stdlib.h>

#include "topics/doc.h"
#include "topics/topics.h"

int NUM_TOPICS; // Declaration of extern global

int main(int argc, char* argv[]) {

    if (argc != 3) {
        fprintf(stderr, "usage: %s <corpus-list-of-files> <num-topics>\n", argv[0]);
        return EXIT_FAILURE;
    }

    NUM_TOPICS = atoi(argv[2]);
    corpus_t corp = load_docs(argv[1]);

    // Corpus-wide topic mixture
    float* alpha = malloc(NUM_TOPICS * sizeof(float));

    // Word-topic affinities
    float* beta = malloc(NUM_TOPICS * dict_len(corp.dictionary) * sizeof(float));

    learn(corp, alpha, beta);

    // TODO
    // Do some sanity checks and make sure we're learning stuff properly.

    return EXIT_SUCCESS;
}
