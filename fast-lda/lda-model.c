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

/*
 * Implemetation of functionality for LDA model construction and MLE.

 */
#include "lda-model.h"
#include "rdtsc-helper.h"


void lda_mle(lda_model* model, lda_suffstats* ss, int estimate_alpha)
{
    int k; int w;

    timer t = start_timer(MLE);

    for (w = 0; w < model->num_terms; w++)
    {
        for (k = 0; k < model->num_topics; k++)
        {
            if (ss->class_word[w * model->num_topics + k] > 0)
            {
                model->log_prob_w[w * model->num_topics + k] =
                log(ss->class_word[w * model->num_topics + k]) -
                log(ss->class_total[k]);
            }
            else
                model->log_prob_w[w * model->num_topics + k] = -100;
        }
    }


    if (estimate_alpha == 1)
    {
        model->alpha = opt_alpha(ss->alpha_suffstats,
           ss->num_docs,
           model->num_topics);

        printf("new alpha = %5.5f\n", model->alpha);
    }

    stop_timer(t);
}

lda_model* new_lda_model(int num_terms, int num_topics)
{
    int i,j;
    lda_model* model;

    model = malloc(sizeof(lda_model));
    model->num_topics = num_topics;
    model->num_terms = num_terms;
    model->alpha = 1.0;
    model->log_prob_w = malloc(sizeof(fp_t) * num_terms * num_topics);
    for (i = 0; i < num_terms; i++)
    {
       for (j = 0; j < num_topics; j++)
           model->log_prob_w[i * num_topics + j] = 0;
   }
   return model;
}


void free_lda_model(lda_model* model)
{
    free(model->log_prob_w);
}


void save_lda_model(lda_model* model, char* model_root)
{
    char filename[100];
    FILE* fileptr;
    int i, j;

    sprintf(filename, "%s.beta", model_root);
    fileptr = fopen(filename, "w");
    for (i = 0; i < model->num_topics; i++)
    {
       for (j = 0; j < model->num_terms; j++)
       {
           fprintf(fileptr, " %5.10f", model->log_prob_w[j * model->num_topics + i]);
       }
       fprintf(fileptr, "\n");
   }
   fclose(fileptr);

   sprintf(filename, "%s.other", model_root);
   fileptr = fopen(filename, "w");
   fprintf(fileptr, "num_topics %d\n", model->num_topics);
   fprintf(fileptr, "num_terms %d\n", model->num_terms);
   fprintf(fileptr, "alpha %5.10f\n", model->alpha);
   fclose(fileptr);
}


lda_model* load_lda_model(char* model_root)
{
    char filename[100];
    FILE* fileptr;
    int i, j, num_terms, num_topics;
    float x, alpha;

    sprintf(filename, "%s.other", model_root);
    printf("loading %s\n", filename);
    fileptr = fopen(filename, "r");
    fscanf(fileptr, "num_topics %d\n", &num_topics);
    fscanf(fileptr, "num_terms %d\n", &num_terms);
    fscanf(fileptr, "alpha %f\n", &alpha);
    fclose(fileptr);

    lda_model* model = new_lda_model(num_terms, num_topics);
    model->alpha = alpha;

    sprintf(filename, "%s.beta", model_root);
    printf("loading %s\n", filename);
    fileptr = fopen(filename, "r");
    for (i = 0; i < num_topics; i++)
    {
        for (j = 0; j < num_terms; j++)
        {
            fscanf(fileptr, "%f", &x);
            model->log_prob_w[j * num_topics + i] = x;
        }
    }
    fclose(fileptr);
    return(model);
}

lda_suffstats* new_lda_suffstats(lda_model* model)
{
    int num_topics = model->num_topics;
    int num_terms = model->num_terms;

    lda_suffstats* ss = malloc(sizeof(lda_suffstats));
    ss->class_total = calloc(num_topics, sizeof(fp_t));
    ss->class_word = calloc(num_terms * num_topics, sizeof(fp_t));

    printf("%d\n", num_terms * num_topics);

    return ss;
}

void zero_initialize_ss(lda_suffstats* ss, lda_model* model)
{
    int k, w;
    for (k = 0; k < model->num_topics; k++)
    {
        for (w = 0; w < model->num_terms; w++)
        {
            ss->class_word[w * model->num_topics + k] = 0;
        }
    }

    for (k = 0; k < model->num_topics; k++) {
        ss->class_total[k] = 0;
    }

    ss->num_docs = 0;
    ss->alpha_suffstats = 0;
}


void random_initialize_ss(lda_suffstats* ss, lda_model* model)
{
    int num_topics = model->num_topics;
    int num_terms = model->num_terms;
    int k, n;
    for (k = 0; k < num_topics; k++)
    {
        for (n = 0; n < num_terms; n++)
        {
            ss->class_word[n * num_topics + k] += 1.0/num_terms + myrand();
            ss->class_total[k] += ss->class_word[n * num_topics + k];
        }
    }
}
