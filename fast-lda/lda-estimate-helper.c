/*
 * Implementation of helper functions for LDA estimation.
 */

#include "lda-estimate-helper.h"

void save_gamma(char* filename, fp_t** gamma, int num_docs, int num_topics)
{
    FILE* fileptr;
    int d, k;
    fileptr = fopen(filename, "w");

    for (d = 0; d < num_docs; d++)
    {
	fprintf(fileptr, "%5.10f", gamma[d][0]);
	for (k = 1; k < num_topics; k++)
	{
	    fprintf(fileptr, " %5.10f", gamma[d][k]);
	}
	fprintf(fileptr, "\n");
    }
    fclose(fileptr);
}

void write_word_assignment(FILE* f, document* doc, fp_t* phi, lda_model* model)
{
    int n;

    fprintf(f, "%03d", doc->length);
    for (n = 0; n < doc->length; n++)
    {
        fprintf(f, " %04d:%02d",
                doc->words[n], argmax(phi + (n * model->num_topics), model->num_topics));
    }
    fprintf(f, "\n");
    fflush(f);
}

int max_corpus_length(corpus* c)
{
    int n, max = 0;
    for (n = 0; n < c->num_docs; n++)
        if (c->docs[n].length > max)
            max = c->docs[n].length;
    return(max);
}

void gatherDocWords(fp_t* entire_matrix, fp_t* reduced_matrix, document* doc, size_t row_size) {
    for (int n = 0; n < doc->length; n++)
    {
        memcpy(reduced_matrix + n * row_size,
               entire_matrix + doc->words[n] * row_size,
               row_size * sizeof(fp_t));
    }
}

void scatterDocWords(fp_t* entire_matrix, fp_t* reduced_matrix, document* doc, size_t row_size) {
    for (int n = 0; n < doc->length; n++)
    {
        entire_matrix[doc->words[n]] = reduced_matrix[n];
    }
}
