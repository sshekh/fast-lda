// (C) Copyright 2004, David M. Blei (blei [at] cs [dot] cmu [dot] edu)

// This file is part of LDA-C.

// LDA-C is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your
// option) any later version.

// LDA-C is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA

#include "lda-data.h"
#include <string.h>

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

int max_corpus_length(corpus* c)
{
    int n, max = 0;
    for (n = 0; n < c->num_docs; n++)
        if (c->docs[n].length > max) max = c->docs[n].length;
    return(max);
}
