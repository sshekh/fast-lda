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

#include "lda-inference.h"
#include <string.h>

fp_t lda_inference(document* doc, lda_model* model, fp_t* var_gamma, fp_t* phi)
{
    int kk;
    __m256i rem;
    STRIDE_SPLIT(model->num_topics, 0, &kk, &rem);

    fp_t converged = 1;
    fp_t phisum = 0, likelihood = 0;
    fp_t likelihood_old = 0, oldphi[model->num_topics];
    int k, n, var_iter;
    fp_t digamma_gam[model->num_topics];

    timer rdtsc = start_timer(LDA_INFERENCE);

    // Initialize the phi for all topics and all words in the doc
    // and compute digamma of the sum of variational gammas over all the topics.
    fp_t init_vga = model->alpha + (doc->total/((fp_t) model->num_topics));
    fp_t init_dgg = digamma(init_vga);
    fp_t init_phi = 1 / (fp_t) model->num_topics;

    for(k = 0 ; k < model->num_topics ; k++) {
        var_gamma[k] = init_vga;
        digamma_gam[k] = init_dgg;
    }

    for (n = 0; n < doc->length; n++) {
        for (k = 0; k < model->num_topics; k ++) {
            phi[n * model->num_topics + k] = init_phi;
        }
    }

    var_iter = 0;

    int kk1;
    __m256i rem1;
    STRIDE_SPLIT(model->num_topics, 1, &kk1, &rem1);

    while ((converged > VAR_CONVERGED) &&
     ((var_iter < VAR_MAX_ITER) || (VAR_MAX_ITER == -1)))
    {
       var_iter++;
       // Update equation (16) for variational phi for each topic and word.
       for (n = 0; n < doc->length; n++)
       {
            // <FL> First backup phi entirely
            memcpy(oldphi, phi + n * model->num_topics, sizeof(fp_t) * model->num_topics);

            // <BG, SS> Moved if else initialization of phisum outside of the loop
            phi[n * model->num_topics + 0] = digamma_gam[0] + model->log_prob_w[doc->words[n] * model->num_topics + 0];
            phisum = phi[n * model->num_topics + 0];

            for (k = 1; k < kk1; k += STRIDE)
            {
                //phi[n * model->num_topics + k] = digamma_gam[k] + model->log_prob_w_doc[n * model->num_topics + k];
                __m256fp dg = _mm256_loadu(digamma_gam + k);
                __m256fp lpwd = _mm256_loadu(model->log_prob_w_doc + (n * model->num_topics + k));

                __m256fp ph = _mm256_add(dg, lpwd);
                _mm256_storeu(phi + (n * model->num_topics + k), ph);
            }
            if (LEFTOVER(model->num_topics, 1)) {
                __m256fp dg = _mm256_maskload(digamma_gam + k, rem1);
                __m256fp lpwd = _mm256_maskload(model->log_prob_w_doc + (n * model->num_topics + k), rem1);

                __m256fp ph = _mm256_add(dg, lpwd);
                _mm256_maskstore(phi + (n * model->num_topics + k), rem1, ph);
            }

            for (k = 1; k < model->num_topics; k++)
            {
                phisum = log_sum(phisum, phi[n * model->num_topics + k]);
            }

            __m256fp doc_counts = _mm256_set1(doc->counts[n]);
            __m256fp ph_sum = _mm256_set1(phisum);

            //Update equation (17) for variational gamma for each topic
            for (k = 0; k < kk; k += STRIDE)
            {
                // Write the final value of the update for phi.
                __m256fp ph = _mm256_loadu(phi + (n * model->num_topics + k));
                __m256fp pho = _mm256_loadu(oldphi + k);
                __m256fp vg = _mm256_loadu(var_gamma + k);

                // phi[n * model->num_topics + k] = exp(phi[n * model->num_topics + k] - phisum);
                __m256fp ph_diff_sum = _mm256_sub(ph, ph_sum);
                ph = _mm256_exp(ph_diff_sum);
                _mm256_storeu(phi + (n * model->num_topics + k), ph);

                // var_gamma[k] = var_gamma[k] + doc->counts[n]*(phi[n * model->num_topics + k] - oldphi[k]);
                __m256fp ph_diff_old = _mm256_sub(ph, pho);
                __m256fp ph_diff_times_doc_counts = _mm256_mul(doc_counts, ph_diff_old);
                vg = _mm256_add(vg, ph_diff_times_doc_counts);
                _mm256_storeu(var_gamma + k, vg);

                // !!! a lot of extra digamma's here because of how we're computing it
                // !!! but its more automatically updated too.
                // digamma_gam[k] = digamma(var_gamma[k]);
                __m256fp dg = digamma_vec(vg);
                _mm256_storeu(digamma_gam + k, dg);
            }

            if (LEFTOVER(model->num_topics, 0))
            {
                // Write the final value of the update for phi.
                __m256fp ph = _mm256_maskload(phi + (n * model->num_topics + k), rem);
                __m256fp pho = _mm256_maskload(oldphi + k, rem);
                __m256fp vg = _mm256_maskload(var_gamma + k, rem);

                // phi[n * model->num_topics + k] = exp(phi[n * model->num_topics + k] - phisum);
                __m256fp ph_diff_sum = _mm256_sub(ph, ph_sum);
                ph = _mm256_exp(ph_diff_sum);
                _mm256_maskstore(phi + (n * model->num_topics + k), rem, ph);

                // var_gamma[k] = var_gamma[k] + doc->counts[n]*(phi[n * model->num_topics + k] - oldphi[k]);
                __m256fp ph_diff_old = _mm256_sub(ph, pho);
                __m256fp ph_diff_times_doc_counts = _mm256_mul(doc_counts, ph_diff_old);
                vg = _mm256_add(vg, ph_diff_times_doc_counts);
                _mm256_maskstore(var_gamma + k, rem, vg);

                // !!! a lot of extra digamma's here because of how we're computing it
                // !!! but its more automatically updated too.
                // digamma_gam[k] = digamma(var_gamma[k]);
                __m256fp dg = digamma_vec(vg);
                _mm256_maskstore(digamma_gam + k, rem, dg);
            }

        }

        likelihood = compute_likelihood(doc, model, phi, var_gamma);
        assert(!isnan(likelihood));
        converged = (likelihood_old - likelihood) / likelihood_old;
        likelihood_old = likelihood;

        // printf("[LDA INF] %8.5f %1.3e\n", likelihood, converged);
    }

    stop_timer(rdtsc);

    timer_manual_increment(INFERENCE_CONVERGE, var_iter);

    return likelihood;
}

fp_t compute_likelihood(document* doc, lda_model* model, fp_t* phi, fp_t* var_gamma)
{
    fp_t likelihood = 0, digsum = 0, var_gamma_sum = 0, dig[model->num_topics];
    int k, n;

    timer rdtsc = start_timer(LIKELIHOOD);

    for (k = 0; k < model->num_topics; k++)
    {
       dig[k] = digamma(var_gamma[k]);
       var_gamma_sum += var_gamma[k];
    }
    digsum = digamma(var_gamma_sum);

    // <BG>: lgamma is a math library function
    likelihood = lgamma(model->alpha * model -> num_topics)
                - model -> num_topics * lgamma(model->alpha)
                - (lgamma(var_gamma_sum));

    // Compute the log likelihood dependent on the variational parameters
    // as per equation (15).
    for (k = 0; k < model->num_topics; k++)
    {
        likelihood += (model->alpha - 1)*(dig[k] - digsum)
                    + lgamma(var_gamma[k])
                    - (var_gamma[k] - 1)*(dig[k] - digsum);
    }

    // <CC> Swapped loop to have the strided access to the transposed
    for (n = 0; n < doc->length; n++)
    {
        for (k = 0; k < model->num_topics; k++)
        {
            if (phi[n * model->num_topics + k] > 0)
            {
                likelihood += doc->counts[n]*
                (phi[n * model->num_topics + k]*((dig[k] - digsum) - log(phi[n * model->num_topics + k])
                    + model->log_prob_w_doc[n * model->num_topics + k]));
            }
        }
    }

    stop_timer(rdtsc);

    return likelihood;
}
