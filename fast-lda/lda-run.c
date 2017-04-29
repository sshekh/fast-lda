/*
 *   This file contains the functionality for running the estimation of the LDA algorithm 
 *   and loading the data. 
 */

#include "lda-run.h"
#include "rdtsc.h"

/*
 * Run LDA estimation. 
 *
 */

int main(int argc, char* argv[])
{
    corpus* corpus;

    long t1;
    (void) time(&t1);
    seedMT(t1);
    seedMT(4357U);

    if (argc > 1)
    {
        if (strcmp(argv[1], "est")==0)
        {
            INITIAL_ALPHA = atof(argv[2]);
            NTOPICS = atoi(argv[3]);
            read_settings(argv[4]);
            corpus = read_data(argv[5]);
            make_directory(argv[7]);

            // <BG> time the whole algorithm
            tsc_counter start, end;
            double cycles = 0.;
            size_t num_runs = 1;

            CPUID(); RDTSC(start);


            run_em(argv[6], argv[7], corpus);

            
            CPUID(); RDTSC(end);

            cycles = (double)(COUNTER_DIFF(end, start))/num_runs;
            printf("Run EM - Runtime [cycles]: %f\n", cycles);

            double flops = 1000;
            printf("Run EM - Performance [flops/cycle]: %f\n", flops/cycles);

        }
    }
    else
    {
        printf("usage : lda est [initial alpha] [k] [settings] [data] [random/seeded/*] [directory]\n");
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

corpus* read_data(char* data_filename)
{
    FILE *fileptr;
    int length, count, word, n, nd, nw;
    corpus* c;

    printf("reading data from %s\n", data_filename);
    c = malloc(sizeof(corpus));
    c->docs = 0;
    c->num_terms = 0;
    c->num_docs = 0;
    fileptr = fopen(data_filename, "r");
    nd = 0; nw = 0;
    while ((fscanf(fileptr, "%10d", &length) != EOF))
    {
    c->docs = (document*) realloc(c->docs, sizeof(document)*(nd+1));
    c->docs[nd].length = length;
    c->docs[nd].total = 0;
    c->docs[nd].words = malloc(sizeof(int)*length);
    c->docs[nd].counts = malloc(sizeof(int)*length);
    for (n = 0; n < length; n++)
    {
        fscanf(fileptr, "%10d:%10d", &word, &count);
        word = word - OFFSET;
        c->docs[nd].words[n] = word;
        c->docs[nd].counts[n] = count;
        c->docs[nd].total += count;
        if (word >= nw) { nw = word + 1; }
    }
    nd++;
    }
    fclose(fileptr);
    c->num_docs = nd;
    c->num_terms = nw;
    printf("number of docs    : %d\n", nd);
    printf("number of terms   : %d\n", nw);
    return(c);
}

