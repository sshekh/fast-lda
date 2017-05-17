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

#ifndef LDA_H
#define LDA_H

#include "rdtsc-helper.h"

typedef struct
{
    int* words;
    int* counts;
    int length;
    int total;
} document;


typedef struct
{
    document* docs;
    int num_terms;
    int num_docs;
} corpus;


typedef struct
{
    fp_t alpha;
    fp_t* log_prob_w;
    fp_t* log_prob_w_doc;
    int num_topics;
    int num_terms;
} lda_model;


typedef struct
{
    fp_t* class_word;
    fp_t* class_total;
    fp_t alpha_suffstats;
    int num_docs;
} lda_suffstats;

#endif
