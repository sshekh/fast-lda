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

#include "lda-estimate.h"
#include "../fast-lda/rdtsc-helper.h"

/*
 * perform inference on a document and update sufficient statistics
 *
 */

fp_t doc_e_step(document* doc, fp_t* gamma, fp_t** phi,
                  lda_model* model, lda_suffstats* ss)
{
    fp_t likelihood;
    int n, k;

    // posterior inference
    timer rdtsc = start_timer(DOC_E_STEP);

    likelihood = lda_inference(doc, model, gamma, phi);

    // update sufficient statistics

    fp_t gamma_sum = 0;
    for (k = 0; k < model->num_topics; k++)
    {
        gamma_sum += gamma[k];
        ss->alpha_suffstats += digamma(gamma[k]);
    }
    ss->alpha_suffstats -= model->num_topics * digamma(gamma_sum);

    for (n = 0; n < doc->length; n++)
    {
        for (k = 0; k < model->num_topics; k++)
        {
            ss->class_word[k][doc->words[n]] += doc->counts[n]*phi[n][k];
            ss->class_total[k] += doc->counts[n]*phi[n][k];
        }
    }

    ss->num_docs = ss->num_docs + 1;

    stop_timer(rdtsc);

    return(likelihood);
}


/*
 * writes the word assignments line for a document to a file
 *
 */

void write_word_assignment(FILE* f, document* doc, fp_t** phi, lda_model* model)
{
    int n;

    fprintf(f, "%03d", doc->length);
    for (n = 0; n < doc->length; n++)
    {
        fprintf(f, " %04d:%02d",
                doc->words[n], argmax(phi[n], model->num_topics));
    }
    fprintf(f, "\n");
    fflush(f);
}


/*
 * saves the gamma parameters of the current dataset
 *
 */

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


/*
 * run_em
 *
 */

void run_em(char* start, char* directory, corpus* corpus)
{

    int d, n;
    lda_model *model = NULL;
    fp_t **var_gamma, **phi;

    // allocate variational parameters
    // Gamma variational parameter for each doc and for each topic which generated the theta
    var_gamma = malloc(sizeof(fp_t*)*(corpus->num_docs));
    for (d = 0; d < corpus->num_docs; d++)
	var_gamma[d] = malloc(sizeof(fp_t) * NTOPICS);

    // Phi variational parameter which generated the z latent variable
    int max_length = max_corpus_length(corpus);
    phi = malloc(sizeof(fp_t*)*max_length);
    for (n = 0; n < max_length; n++)
	phi[n] = malloc(sizeof(fp_t) * NTOPICS);

    // initialize model

    char filename[1000];

    lda_suffstats* ss = NULL;
    if (strcmp(start, "seeded")==0)
    {
        model = new_lda_model(corpus->num_terms, NTOPICS);
        ss = new_lda_suffstats(model);
        corpus_initialize_ss(ss, model, corpus);
        lda_mle(model, ss, 0);
        model->alpha = INITIAL_ALPHA;
    }
    else if (strcmp(start, "random")==0)
    {
        model = new_lda_model(corpus->num_terms, NTOPICS);
    }
    else
    {
        model = load_lda_model(start);
        ss = new_lda_suffstats(model);
    }
    

    // run expectation maximization

    timer rdtsc = start_timer(RUN_EM);
    ss = new_lda_suffstats(model);
    random_initialize_ss(ss, model);
    lda_mle(model, ss, 0);
    model->alpha = INITIAL_ALPHA;

    int i = 0;
    fp_t likelihood, likelihood_old = 0, converged = 1;

    while (((converged < 0) || (converged > EM_CONVERGED) || (i <= 2)) && (i <= EM_MAX_ITER))
    {
        i++; printf("**** em iteration %d ****\n", i);
        likelihood = 0;
        zero_initialize_ss(ss, model);

        // e-step

        for (d = 0; d < corpus->num_docs; d++)
        {
            if ((d % 1000) == 0) printf("document %d\n",d);
            likelihood += doc_e_step(&(corpus->docs[d]),
                                     var_gamma[d],
                                     phi,
                                     model,
                                     ss);
        }

        // m-step

        lda_mle(model, ss, ESTIMATE_ALPHA);

        // check for convergence

        converged = (likelihood_old - likelihood) / (likelihood_old);
        if (converged < 0) VAR_MAX_ITER = VAR_MAX_ITER * 2;
        likelihood_old = likelihood;
    }

    stop_timer(rdtsc);

    timer_manual_increment(EM_CONVERGE, i);

    // output the final model

    sprintf(filename,"%s/final",directory);
    save_lda_model(model, filename);
    sprintf(filename,"%s/final.gamma",directory);
    save_gamma(filename, var_gamma, corpus->num_docs, model->num_topics);

    // <SS>: output the word assignments (for visualization) were removed
}


/*
 * read settings.
 *
 */

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


/*
 * inference only
 *
 */

void infer(char* model_root, char* save, corpus* corpus)
{
    FILE* fileptr;
    char filename[100];
    int i, d, n;
    lda_model *model;
    fp_t **var_gamma, likelihood, **phi;
    document* doc;

    model = load_lda_model(model_root);
    var_gamma = malloc(sizeof(fp_t*)*(corpus->num_docs));
    for (i = 0; i < corpus->num_docs; i++)
	var_gamma[i] = malloc(sizeof(fp_t)*model->num_topics);
    sprintf(filename, "%s-lda-lhood.dat", save);
    fileptr = fopen(filename, "w");
    for (d = 0; d < corpus->num_docs; d++)
    {
	if (((d % 100) == 0) && (d>0)) printf("document %d\n",d);

	doc = &(corpus->docs[d]);
	phi = (fp_t**) malloc(sizeof(fp_t*) * doc->length);
	for (n = 0; n < doc->length; n++)
	    phi[n] = (fp_t*) malloc(sizeof(fp_t) * model->num_topics);
	likelihood = lda_inference(doc, model, var_gamma[d], phi);

	fprintf(fileptr, "%5.5f\n", likelihood);
    }
    fclose(fileptr);
    sprintf(filename, "%s-gamma.dat", save);
    save_gamma(filename, var_gamma, corpus->num_docs, model->num_topics);
}


/*
 * update sufficient statistics
 *
 */



/*
 * main
 *
 */

int main(int argc, char* argv[])
{
    // (est / inf) alpha k settings data (random / seed/ model) (directory / out)

    corpus* corpus;

    long t1;
    (void) time(&t1);
    seedMT(t1);
    seedMT(4357U);

    if (argc > 1)
    {
        if (strcmp(argv[1], "est")==0)
        {
            int doc_limit = atoi(argv[2]);
            INITIAL_ALPHA = atof(argv[3]);
            NTOPICS = atoi(argv[4]);
            read_settings(argv[5]);
            corpus = read_data(argv[6], doc_limit);
            run_em(argv[7], argv[8], corpus);
        }
        if (strcmp(argv[1], "inf")==0)
        {
            read_settings(argv[2]);
            corpus = read_data(argv[4], -1);
            infer(argv[3], argv[5], corpus);
        }
    }
    else
    {
        printf("usage : lda est [ndocs] [initial alpha] [k] [settings] [data] [random/seeded/*] [directory]\n");
        printf("        lda inf [settings] [model] [data] [name]\n");
    }

    FILE* f;
    if (strcmp(argv[9], "-out") == 0) {
        f = stdout;
    } else {
        f = fopen(argv[9],"w");
    }

    print_timings(f);
    fclose(f);

    return(0);
}
