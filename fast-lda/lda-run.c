/*
 *   This file contains the functionality for running the estimation of the LDA algorithm
 *   and loading the data.
 */

#include "lda-run.h"
#include "rdtsc-helper.h"

/*
 * Run LDA estimation.
 *
 */

int main(int argc, char* argv[])
{
    corpus* corpus;

    // long t1;
    // (void) time(&t1);
    // seedMT(t1);
    seedMT(4357U);

    if (argc > 8)
    {
        if (strcmp(argv[1], "est")==0)
        {
            int doc_limit = atoi(argv[2]);
            INITIAL_ALPHA = atof(argv[3]);
            NTOPICS = atoi(argv[4]);
            read_settings(argv[5]);
            corpus = read_data(argv[6], doc_limit);

            init_timing_infrastructure();
            run_em(argv[7], argv[8], corpus);

            FILE* f;
            if (strcmp(argv[9], "-out") == 0) {
                f = stdout;
            } else {
                f = fopen(argv[9], "w");
            }

            print_timings(f);
            fclose(f);

        }
    }
    else
    {
        printf("usage : lda est [ndocs] [initial alpha] [k] [settings] [data] [random/seeded/*] [directory] [-out]\n");
    }
    return(0);
}

void read_settings(char* filename)
{
    FILE* fileptr;
    char alpha_action[100];
    fileptr = fopen(filename, "r");
    fscanf(fileptr, "var max iter %d\n", &VAR_MAX_ITER);
    fscanf(fileptr, "var convergence %f\n", &VAR_CONVERGED);
    fscanf(fileptr, "em max iter %d\n", &EM_MAX_ITER);
    fscanf(fileptr, "em convergence %f\n", &EM_CONVERGED);
    fscanf(fileptr, "alpha %s", alpha_action);
    if (strcmp(alpha_action, "fixed")==0)
    {
        ESTIMATE_ALPHA = 0;
    }
    else
    {
        ESTIMATE_ALPHA = 1;
    }
    fclose(fileptr);
}

corpus* read_data(char* data_filename, int doc_limit)
{
    FILE *fileptr;
    int length, count, word, n, nd;
    corpus* c;

    printf("reading data from %s\n", data_filename);
    c = _mm_malloc(sizeof(corpus), ALIGNMENT);
    c->docs = 0;
    c->num_terms = 0;
    c->num_docs = 0;
    fileptr = fopen(data_filename, "r");
    nd = 0;


    while ((fscanf(fileptr, "%10d", &length) != EOF) && (doc_limit < 1 || nd < doc_limit))
    {
        c->docs = (document*) realloc(c->docs, sizeof(document)*(nd+1));
        c->docs[nd].length = length;
        c->docs[nd].total = 0;
        c->docs[nd].words = _mm_malloc(sizeof(int)*length, ALIGNMENT);
        c->docs[nd].counts = _mm_malloc(sizeof(int)*length, ALIGNMENT);
        for (n = 0; n < length; n++)
        {
            fscanf(fileptr, "%10d:%10d", &word, &count);
            word = word - OFFSET;
            c->docs[nd].words[n] = word;
            c->docs[nd].counts[n] = count;
            c->docs[nd].total += count;
        }
        nd++;
    }


    fclose(fileptr);
    c->num_docs = nd;

    // Hack / convention
    int ln = strlen(data_filename);
    data_filename[ln - 3] = 'v';
    data_filename[ln - 2] = 'c';
    data_filename[ln - 1] = 'b';

    fileptr = fopen(data_filename, "r");
    char unused[300];
    while (fgets(unused, sizeof(unused), fileptr) != NULL) {
        c->num_terms++;
    }

    fclose(fileptr);

    printf("number of docs    : %d\n", nd);
    printf("number of terms   : %d\n", c->num_terms);
    return(c);
}

