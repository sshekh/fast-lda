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
 * Implementation of the LDA parameter estimation functionality.
 */

#include "fp.h"
#include "lda-estimate.h"
#include "lda-estimate-helper.h"
#include "rdtsc-helper.h"

#define LAG 5

fp_t doc_e_step(document* doc, fp_t* gamma, fp_t* phi,
                  lda_model* model, lda_suffstats* ss)
{
    timer rdtsc = start_timer(DOC_E_STEP);

    fp_t likelihood;
    int n, k;

    // <CC> Gather the rows from large matrices for this doc
    // for optimization 2.
    gatherDocWords(model->log_prob_w, model->log_prob_w_doc, doc,
                   model->num_topics);

    // Posterior variational inference.
    likelihood = lda_inference(doc, model, gamma, phi);

    // <CC> Scatter the rows used for this doc in the original matrix
    // for optimization 2.
    scatterDocWords(model->log_prob_w, model->log_prob_w_doc, doc,
                    model->num_topics);

    int KK;
    __m256i KMASK;
    STRIDE_SPLIT(model->num_topics, 0, &KK, &KMASK);

    // Update sufficient statistics.
    __m256fp gamma_accs = _mm256_setzero();
    __m256fp alpha_accs = _mm256_setzero();
    for (k = 0; k < KK; k += STRIDE)
    {
        __m256fp gams = _mm256_loadu(gamma + k);
        //gamma_sum += gamma[k];
        gamma_accs = _mm256_add(gamma_accs, gams);

        //ss->alpha_suffstats += digamma(gamma[k]);
        __m256fp digams = digamma_vec(gams);
        alpha_accs = _mm256_add(alpha_accs, digams);
    }
    if (LEFTOVER(model->num_topics, 0)) {
        __m256fp gams = _mm256_maskload(gamma + KK, KMASK);

        __m256fp dig_unmasked = digamma_vec(gams);
        __m256fp digams = _mm256_and(dig_unmasked, _mm256_castsi256(KMASK));

        gamma_accs = _mm256_add(gamma_accs, gams);
        alpha_accs = _mm256_add(alpha_accs, digams);
    }
    // Collect all the partial sums
    __m256fp gamma_totals = hsum(gamma_accs);
    __m256fp alpha_totals = hsum(alpha_accs);
    fp_t gamma_sum = first(gamma_totals);

    //Update alpha.
    ss->alpha_suffstats += first(alpha_totals);
    ss->alpha_suffstats -= model->num_topics * digamma(gamma_sum);

    // <CC> Tile by 2 on n (two words in parallel)
    int tiling_factor = 2;
    for (n = 0; n + tiling_factor - 1 < doc->length; n+=tiling_factor)
    {

        int di1 = doc->words[n] * model->num_topics;
        int di2 = doc->words[n + 1] * model->num_topics;

        int ni1 = n * model->num_topics;
        int ni2 = (n + 1) * model->num_topics;

        __m256fp doc_counts1 = _mm256_set1(doc->counts[n]);
        __m256fp doc_counts2 = _mm256_set1(doc->counts[n + 1]);
        for (k = 0; k < KK; k += STRIDE)
        {
            __m256fp cw1 = _mm256_loadu(ss->class_word + di1 + k);
            __m256fp cw2 = _mm256_loadu(ss->class_word + di2 + k);
            
            __m256fp ct = _mm256_loadu(ss->class_total + k);

            __m256fp ph1 = _mm256_loadu(phi + ni1 + k);
            __m256fp ph2 = _mm256_loadu(phi + ni2 + k);

            //ss->class_word[di + k] += doc->counts[n]*phi[ni + k];
            cw1 = _mm256_fmadd(doc_counts1, ph1, cw1);
            cw2 = _mm256_fmadd(doc_counts2, ph2, cw2);


            //ss->class_total[k] += doc->counts[n]*phi[ni + k];
            ct = _mm256_fmadd(doc_counts1, ph1, ct);
            ct = _mm256_fmadd(doc_counts2, ph2, ct);

            _mm256_storeu(ss->class_word + di1 + k, cw1);
            _mm256_storeu(ss->class_word + di2 + k, cw2);

            _mm256_storeu(ss->class_total + k, ct);
        }
 
        if (LEFTOVER(model->num_topics, 0)) {
            __m256fp cw1 = _mm256_maskload(ss->class_word + di1 + KK, KMASK);
            __m256fp cw2 = _mm256_maskload(ss->class_word + di2 + KK, KMASK);

            __m256fp ct = _mm256_maskload(ss->class_total + KK, KMASK);

            __m256fp ph1 = _mm256_maskload(phi + ni1 + KK, KMASK);
            __m256fp ph2 = _mm256_maskload(phi + ni2 + KK, KMASK);
 
            //ss->class_word[di + k] += doc->counts[n]*phi[ni + k];
            cw1 = _mm256_fmadd(doc_counts1, ph1, cw1);
            cw2 = _mm256_fmadd(doc_counts2, ph2, cw2);
 
            //ss->class_total[k] += doc->counts[n]*phi[ni + k];
            ct = _mm256_fmadd(doc_counts1, ph1, ct);
            ct = _mm256_fmadd(doc_counts2, ph2, ct);
 
            _mm256_maskstore(ss->class_word + di1 + KK, KMASK, cw1);
            _mm256_maskstore(ss->class_word + di2 + KK, KMASK, cw2);
            _mm256_maskstore(ss->class_total + KK, KMASK, ct);
        }
     }

     //Update beta.
    for (; n < doc->length; n++)
    {
        int di = doc->words[n] * model->num_topics;
        int ni = n * model->num_topics;
        __m256fp doc_counts = _mm256_set1(doc->counts[n]);
        for (k = 0; k < KK; k += STRIDE)
        {
            __m256fp cw = _mm256_loadu(ss->class_word + di + k);
            __m256fp ct = _mm256_loadu(ss->class_total + k);
            __m256fp ph = _mm256_loadu(phi + ni + k);

            //ss->class_word[di + k] += doc->counts[n]*phi[ni + k];
            cw = _mm256_fmadd(doc_counts, ph, cw);

            //ss->class_total[k] += doc->counts[n]*phi[ni + k];
            ct = _mm256_fmadd(doc_counts, ph, ct);

            _mm256_storeu(ss->class_word + di + k, cw);
            _mm256_storeu(ss->class_total + k, ct);
        }

        if (LEFTOVER(model->num_topics, 0)) {
            __m256fp cw1 = _mm256_maskload(ss->class_word + di1 + KK, KMASK);
            __m256fp cw2 = _mm256_maskload(ss->class_word + di2 + KK, KMASK);

            __m256fp ct = _mm256_maskload(ss->class_total + KK, KMASK);
            __m256fp ph = _mm256_maskload(phi + ni + KK, KMASK);

            //ss->class_word[di + k] += doc->counts[n]*phi[ni + k];
            cw = _mm256_fmadd(doc_counts, ph, cw);

            //ss->class_total[k] += doc->counts[n]*phi[ni + k];
            ct = _mm256_fmadd(doc_counts, ph, ct);

            _mm256_maskstore(ss->class_word + di + KK, KMASK, cw);
            _mm256_maskstore(ss->class_total + KK, KMASK, ct);
        }
    }

    ss->num_docs = ss->num_docs + 1;

    stop_timer(rdtsc);

    return(likelihood);
}


void run_em(char* start, char* directory, corpus* corpus)
{
    printf("start em\n");
    int d;
    lda_model *model = NULL;
    // Variational parameters
    fp_t **var_gamma, *phi;

    // Gamma variational parameter for each doc and for each topic.
    var_gamma = _mm_malloc(sizeof(fp_t*)*(corpus->num_docs), ALIGNMENT);
    for (d = 0; d < corpus->num_docs; d++)
        var_gamma[d] = _mm_malloc(sizeof(fp_t) * NTOPICS, ALIGNMENT);

    // Phi variational parameter for each term in the vocabulary and for each topic.
    int max_length = max_corpus_length(corpus);
    phi = _mm_malloc(sizeof(fp_t) * max_length * NTOPICS, ALIGNMENT);

    // initialize model
    char filename[1000];

    lda_suffstats* ss = NULL;
    if (strcmp(start, "random")==0)
    {
        printf("Init model\n");
        model = new_lda_model(corpus->num_terms, NTOPICS, max_length);
    }
    else
    {
        model = load_lda_model(start);
        ss = new_lda_suffstats(model);
    }

    sprintf(filename,"%s/000",directory);
    save_lda_model(model, filename, max_length);


    // run expectation maximization
    int var_iter = 0;
    fp_t likelihood, likelihood_old = 0, converged = 1;

    timer rdtsc = start_timer(RUN_EM);
    ss = new_lda_suffstats(model);
    random_initialize_ss(ss, model);
    lda_mle(model, ss, 0);
    model->alpha = INITIAL_ALPHA;

    while (((converged < 0) || (converged > EM_CONVERGED) || (var_iter <= 2)) && (var_iter <= EM_MAX_ITER))
    {
        var_iter++;
        printf("**** em iteration %d ****\n", var_iter);

        likelihood = 0;
        zero_initialize_ss(ss, model);

        // E-Step performed for each document.
        for (d = 0; d < corpus->num_docs; d++)
        {
            if ((d % 1000) == 0) printf("document %d\n",d);
            likelihood += doc_e_step(&(corpus->docs[d]),
                                     var_gamma[d],
                                     phi,
                                     model,
                                     ss);
        }

        // M-Step
        lda_mle(model, ss, ESTIMATE_ALPHA);

        // check for convergence
        converged = (likelihood_old - likelihood) / (likelihood_old);
        if (converged < 0) VAR_MAX_ITER = VAR_MAX_ITER * 2;
        likelihood_old = likelihood;
    }

    stop_timer(rdtsc);

    timer_manual_increment(EM_CONVERGE, var_iter);

    // output the final model
    sprintf(filename,"%s/final",directory);
    save_lda_model(model, filename, max_length);
    sprintf(filename,"%s/final.gamma",directory);
    save_gamma(filename, var_gamma, corpus->num_docs, model->num_topics);

    // <BG>: output the word assignments (for visualization) were removed
}
