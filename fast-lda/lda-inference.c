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

    int kk1;
    __m256i rem1;
    STRIDE_SPLIT(model->num_topics, 1, &kk1, &rem1);

    fp_t converged = 1;
    fp_t likelihood = 0;
    fp_t likelihood_old = 0;
    int k, n, var_iter;

    fp_t oldphi0[model->num_topics];
    fp_t oldphi1[model->num_topics];
    fp_t oldphi2[model->num_topics];
    fp_t oldphi3[model->num_topics];

    fp_t phisum0 = 0;
    fp_t phisum1 = 0;
    fp_t phisum2 = 0;
    fp_t phisum3 = 0;

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

    int rest = doc->length % 4;

    while ((converged > VAR_CONVERGED) &&
     ((var_iter < VAR_MAX_ITER) || (VAR_MAX_ITER == -1)))
    {
       var_iter++;
       // Update equation (16) for variational phi for each topic and word.
       for (n = 0; n < doc->length - rest; n += 4)
       {
            // <FL> First backup phi entirely
            memcpy(oldphi0, phi + (n + 0) * model->num_topics, sizeof(fp_t) * model->num_topics);
            memcpy(oldphi1, phi + (n + 1) * model->num_topics, sizeof(fp_t) * model->num_topics);
            memcpy(oldphi2, phi + (n + 2) * model->num_topics, sizeof(fp_t) * model->num_topics);
            memcpy(oldphi3, phi + (n + 3) * model->num_topics, sizeof(fp_t) * model->num_topics);

            // <BG, SS> Moved if else initialization of phisum outside of the loop
            phi[(n + 0) * model->num_topics + 0] = digamma_gam[0] + model->log_prob_w_doc[(n + 0) * model->num_topics + 0];
            phi[(n + 1) * model->num_topics + 0] = digamma_gam[0] + model->log_prob_w_doc[(n + 1) * model->num_topics + 0];
            phi[(n + 2) * model->num_topics + 0] = digamma_gam[0] + model->log_prob_w_doc[(n + 2) * model->num_topics + 0];
            phi[(n + 3) * model->num_topics + 0] = digamma_gam[0] + model->log_prob_w_doc[(n + 3) * model->num_topics + 0];

            // phisum0 = phi[(n + 0) * model->num_topics + 0];
            // phisum1 = phi[(n + 1) * model->num_topics + 0];
            // phisum2 = phi[(n + 2) * model->num_topics + 0];
            // phisum3 = phi[(n + 3) * model->num_topics + 0];

            for (k = 1; k < kk1; k += STRIDE)
            {
                //phi[n * model->num_topics + k] = digamma_gam[k] + model->log_prob_w_doc[n * model->num_topics + k];
                __m256fp dg = _mm256_loadu(digamma_gam + k);
                __m256fp lpwd0 = _mm256_loadu(model->log_prob_w_doc + ((n + 0) * model->num_topics + k));
                __m256fp lpwd1 = _mm256_loadu(model->log_prob_w_doc + ((n + 1) * model->num_topics + k));
                __m256fp lpwd2 = _mm256_loadu(model->log_prob_w_doc + ((n + 2) * model->num_topics + k));
                __m256fp lpwd3 = _mm256_loadu(model->log_prob_w_doc + ((n + 3) * model->num_topics + k));

                __m256fp ph0 = _mm256_add(dg, lpwd0);
                __m256fp ph1 = _mm256_add(dg, lpwd1);
                __m256fp ph2 = _mm256_add(dg, lpwd2);
                __m256fp ph3 = _mm256_add(dg, lpwd3);

                _mm256_storeu(phi + ((n + 0) * model->num_topics + k), ph0);
                _mm256_storeu(phi + ((n + 1) * model->num_topics + k), ph1);
                _mm256_storeu(phi + ((n + 2) * model->num_topics + k), ph2);
                _mm256_storeu(phi + ((n + 3) * model->num_topics + k), ph3);
            }

            if (LEFTOVER(model->num_topics, 1)) {
                __m256fp dg = _mm256_maskload(digamma_gam + k, rem1);
                __m256fp lpwd0 = _mm256_maskload(model->log_prob_w_doc + ((n + 0) * model->num_topics + k), rem1);
                __m256fp lpwd1 = _mm256_maskload(model->log_prob_w_doc + ((n + 1) * model->num_topics + k), rem1);
                __m256fp lpwd2 = _mm256_maskload(model->log_prob_w_doc + ((n + 2) * model->num_topics + k), rem1);
                __m256fp lpwd3 = _mm256_maskload(model->log_prob_w_doc + ((n + 3) * model->num_topics + k), rem1);

                __m256fp ph0 = _mm256_add(dg, lpwd0);
                __m256fp ph1 = _mm256_add(dg, lpwd1);
                __m256fp ph2 = _mm256_add(dg, lpwd2);
                __m256fp ph3 = _mm256_add(dg, lpwd3);

                _mm256_maskstore(phi + ((n + 0) * model->num_topics + k), rem1, ph0);
                _mm256_maskstore(phi + ((n + 1) * model->num_topics + k), rem1, ph1);
                _mm256_maskstore(phi + ((n + 2) * model->num_topics + k), rem1, ph2);
                _mm256_maskstore(phi + ((n + 3) * model->num_topics + k), rem1, ph3);
            }

            
            int kkSTRIDE;
            __m256i remSTRIDE;
            STRIDE_SPLIT(model->num_topics, STRIDE, &kkSTRIDE, &remSTRIDE);

            __m256fp v_phsum0 = _mm256_loadu(phi + (n * model->num_topics));
            __m256fp v_phsum1 = _mm256_loadu(phi + ((n + 1) * model->num_topics));
            __m256fp v_phsum2 = _mm256_loadu(phi + ((n + 2) * model->num_topics));
            __m256fp v_phsum3 = _mm256_loadu(phi + ((n + 3) * model->num_topics));


            for (k = STRIDE; k < kkSTRIDE; k += STRIDE)
            {
                __m256fp ph0 = _mm256_loadu(phi + (n * model->num_topics + k));
                __m256fp ph1 = _mm256_loadu(phi + ((n + 1) * model->num_topics + k));
                __m256fp ph2 = _mm256_loadu(phi + ((n + 2) * model->num_topics + k));
                __m256fp ph3 = _mm256_loadu(phi + ((n + 3) * model->num_topics + k));

                v_phsum0 = log_sum_vec(v_phsum0, ph0);
                v_phsum1 = log_sum_vec(v_phsum1, ph1);
                v_phsum2 = log_sum_vec(v_phsum2, ph2);
                v_phsum3 = log_sum_vec(v_phsum3, ph3);

            }
            if (LEFTOVER(model->num_topics, STRIDE))
            {
                __m256fp ph0 = _mm256_maskload(phi + (n * model->num_topics + k), remSTRIDE);
                __m256fp ph1 = _mm256_maskload(phi + ((n + 1) * model->num_topics + k), remSTRIDE);
                __m256fp ph2 = _mm256_maskload(phi + ((n + 2) * model->num_topics + k), remSTRIDE);
                __m256fp ph3 = _mm256_maskload(phi + ((n + 3) * model->num_topics + k), remSTRIDE);

                v_phsum0 = log_sum_vec_masked(v_phsum0, ph0, remSTRIDE);
                v_phsum1 = log_sum_vec_masked(v_phsum1, ph1, remSTRIDE);
                v_phsum2 = log_sum_vec_masked(v_phsum2, ph2, remSTRIDE);
                v_phsum3 = log_sum_vec_masked(v_phsum3, ph3, remSTRIDE);
            }
            
             // <CC> Finilized the phi_sum

            double ph_fisrt_half_sum0 = log_sum(((fp_t *)(&v_phsum0))[0], ((fp_t *)(&v_phsum0))[1]);
            double ph_second_half_sum0 = log_sum(((fp_t *)(&v_phsum0))[2], ((fp_t *)(&v_phsum0))[3]);
            phisum0 = log_sum(ph_fisrt_half_sum0, ph_second_half_sum0); 


            double ph_fisrt_half_sum1 = log_sum(((fp_t *)(&v_phsum1))[0], ((fp_t *)(&v_phsum1))[1]);
            double ph_second_half_sum1 = log_sum(((fp_t *)(&v_phsum1))[2], ((fp_t *)(&v_phsum1))[3]);
            phisum1 = log_sum(ph_fisrt_half_sum1, ph_second_half_sum1);    

            double ph_fisrt_half_sum2 = log_sum(((fp_t *)(&v_phsum2))[0], ((fp_t *)(&v_phsum2))[1]);
            double ph_second_half_sum2 = log_sum(((fp_t *)(&v_phsum2))[2], ((fp_t *)(&v_phsum2))[3]);
            phisum2 = log_sum(ph_fisrt_half_sum2, ph_second_half_sum2);    

            double ph_fisrt_half_sum3 = log_sum(((fp_t *)(&v_phsum3))[0], ((fp_t *)(&v_phsum3))[1]);
            double ph_second_half_sum3 = log_sum(((fp_t *)(&v_phsum3))[2], ((fp_t *)(&v_phsum3))[3]);
            phisum3 = log_sum(ph_fisrt_half_sum3, ph_second_half_sum3);    
   

            __m256fp doc_counts0 = _mm256_set1(doc->counts[(n + 0)]);
            __m256fp doc_counts1 = _mm256_set1(doc->counts[(n + 1)]);
            __m256fp doc_counts2 = _mm256_set1(doc->counts[(n + 2)]);
            __m256fp doc_counts3 = _mm256_set1(doc->counts[(n + 3)]);

            __m256fp ph_sum0 = _mm256_set1(phisum0);
            __m256fp ph_sum1 = _mm256_set1(phisum1);
            __m256fp ph_sum2 = _mm256_set1(phisum2);
            __m256fp ph_sum3 = _mm256_set1(phisum3);

            //Update equation (17) for variational gamma for each topic
            for (k = 0; k < kk; k += STRIDE)
            {
                // Write the final value of the update for phi.
                __m256fp ph0 = _mm256_loadu(phi + ((n + 0) * model->num_topics + k));
                __m256fp ph1 = _mm256_loadu(phi + ((n + 1) * model->num_topics + k));
                __m256fp ph2 = _mm256_loadu(phi + ((n + 2) * model->num_topics + k));
                __m256fp ph3 = _mm256_loadu(phi + ((n + 3) * model->num_topics + k));

                __m256fp pho0 = _mm256_loadu(oldphi0 + k);
                __m256fp pho1 = _mm256_loadu(oldphi1 + k);
                __m256fp pho2 = _mm256_loadu(oldphi2 + k);
                __m256fp pho3 = _mm256_loadu(oldphi3 + k);

                __m256fp vg = _mm256_loadu(var_gamma + k);

                // phi[n * model->num_topics + k] = exp(phi[n * model->num_topics + k] - phisum);
                __m256fp ph_diff_sum0 = _mm256_sub(ph0, ph_sum0);
                __m256fp ph_diff_sum1 = _mm256_sub(ph1, ph_sum1);
                __m256fp ph_diff_sum2 = _mm256_sub(ph2, ph_sum2);
                __m256fp ph_diff_sum3 = _mm256_sub(ph3, ph_sum3);

                ph0 = _mm256_exp(ph_diff_sum0);
                ph1 = _mm256_exp(ph_diff_sum1);
                ph2 = _mm256_exp(ph_diff_sum2);
                ph3 = _mm256_exp(ph_diff_sum3);

                _mm256_storeu(phi + ((n + 0) * model->num_topics + k), ph0);
                _mm256_storeu(phi + ((n + 1) * model->num_topics + k), ph1);
                _mm256_storeu(phi + ((n + 2) * model->num_topics + k), ph2);
                _mm256_storeu(phi + ((n + 3) * model->num_topics + k), ph3);

                // var_gamma[k] = var_gamma[k] + doc->counts[n]*(phi[n * model->num_topics + k] - oldphi[k]);
                __m256fp ph_diff_old0 = _mm256_sub(ph0, pho0);
                __m256fp ph_diff_old1 = _mm256_sub(ph1, pho1);
                __m256fp ph_diff_old2 = _mm256_sub(ph2, pho2);
                __m256fp ph_diff_old3 = _mm256_sub(ph3, pho3);

                // <FL> Not the best but var_gamma is one vector and we have 4 other ones here.
                vg = _mm256_fmadd(doc_counts0, ph_diff_old0, vg);
                vg = _mm256_fmadd(doc_counts1, ph_diff_old1, vg);
                vg = _mm256_fmadd(doc_counts2, ph_diff_old2, vg);
                vg = _mm256_fmadd(doc_counts3, ph_diff_old3, vg);

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
                __m256fp ph0 = _mm256_maskload(phi + ((n + 0) * model->num_topics + k), rem);
                __m256fp ph1 = _mm256_maskload(phi + ((n + 1) * model->num_topics + k), rem);
                __m256fp ph2 = _mm256_maskload(phi + ((n + 2) * model->num_topics + k), rem);
                __m256fp ph3 = _mm256_maskload(phi + ((n + 3) * model->num_topics + k), rem);

                __m256fp pho0 = _mm256_maskload(oldphi0 + k, rem);
                __m256fp pho1 = _mm256_maskload(oldphi1 + k, rem);
                __m256fp pho2 = _mm256_maskload(oldphi2 + k, rem);
                __m256fp pho3 = _mm256_maskload(oldphi3 + k, rem);

                __m256fp vg = _mm256_maskload(var_gamma + k, rem);

                // phi[n * model->num_topics + k] = exp(phi[n * model->num_topics + k] - phisum);
                __m256fp ph_diff_sum0 = _mm256_sub(ph0, ph_sum0);
                __m256fp ph_diff_sum1 = _mm256_sub(ph1, ph_sum1);
                __m256fp ph_diff_sum2 = _mm256_sub(ph2, ph_sum2);
                __m256fp ph_diff_sum3 = _mm256_sub(ph3, ph_sum3);

                ph0 = _mm256_exp(ph_diff_sum0);
                ph1 = _mm256_exp(ph_diff_sum1);
                ph2 = _mm256_exp(ph_diff_sum2);
                ph3 = _mm256_exp(ph_diff_sum3);

                _mm256_maskstore(phi + ((n + 0) * model->num_topics + k), rem, ph0);
                _mm256_maskstore(phi + ((n + 1) * model->num_topics + k), rem, ph1);
                _mm256_maskstore(phi + ((n + 2) * model->num_topics + k), rem, ph2);
                _mm256_maskstore(phi + ((n + 3) * model->num_topics + k), rem, ph3);

                // var_gamma[k] = var_gamma[k] + doc->counts[n]*(phi[n * model->num_topics + k] - oldphi[k]);
                __m256fp ph_diff_old0 = _mm256_sub(ph0, pho0);
                __m256fp ph_diff_old1 = _mm256_sub(ph1, pho1);
                __m256fp ph_diff_old2 = _mm256_sub(ph2, pho2);
                __m256fp ph_diff_old3 = _mm256_sub(ph3, pho3);

                vg = _mm256_fmadd(doc_counts0, ph_diff_old0, vg);
                vg = _mm256_fmadd(doc_counts1, ph_diff_old1, vg);
                vg = _mm256_fmadd(doc_counts2, ph_diff_old2, vg);
                vg = _mm256_fmadd(doc_counts3, ph_diff_old3, vg);

                _mm256_maskstore(var_gamma + k, rem, vg);

                // !!! a lot of extra digamma's here because of how we're computing it
                // !!! but its more automatically updated too.
                // digamma_gam[k] = digamma(var_gamma[k]);
                __m256fp dg = digamma_vec(vg);
                _mm256_maskstore(digamma_gam + k, rem, dg);
            }

        }
        // Now do the remaining doc->length % 4 rows, one by one.
        for (; n < doc->length ; n++) {

            // <FL> First backup phi entirely
            memcpy(oldphi0, phi + n * model->num_topics, sizeof(fp_t) * model->num_topics);
            // <BG, SS> Moved if else initialization of phisum outside of the loop
            phi[n * model->num_topics + 0] = digamma_gam[0] + model->log_prob_w[doc->words[n] * model->num_topics + 0];
            phisum0 = phi[n * model->num_topics + 0];

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


            int kkSTRIDE;
            __m256i remSTRIDE;
            STRIDE_SPLIT(model->num_topics, STRIDE, &kkSTRIDE, &remSTRIDE);

            __m256fp v_phsum = _mm256_loadu(phi + (n * model->num_topics));

            for (k = STRIDE; k < kkSTRIDE; k += STRIDE)
            {
                __m256fp ph = _mm256_loadu(phi + (n * model->num_topics + k));
                v_phsum = log_sum_vec(v_phsum, ph);
            }
            if (LEFTOVER(model->num_topics, STRIDE))
            {
                __m256fp ph = _mm256_maskload(phi + (n * model->num_topics + k), remSTRIDE);
                v_phsum = log_sum_vec_masked(v_phsum, ph, remSTRIDE);
            }
            // <CC> Finilized the phi_sum

            double some_sum0 = log_sum(((fp_t *)(&v_phsum))[0], ((fp_t *)(&v_phsum))[1]);
            double some_sum1 = log_sum(((fp_t *)(&v_phsum))[2], ((fp_t *)(&v_phsum))[3]);

            phisum0 = log_sum(some_sum0, some_sum1);

            __m256fp doc_counts = _mm256_set1(doc->counts[n]);
            __m256fp ph_sum = _mm256_set1(phisum0);

            //Update equation (17) for variational gamma for each topic
            for (k = 0; k < kk; k += STRIDE)
            {
                // Write the final value of the update for phi.
                __m256fp ph = _mm256_loadu(phi + (n * model->num_topics + k));
                __m256fp pho = _mm256_loadu(oldphi0 + k);
                __m256fp vg = _mm256_loadu(var_gamma + k);

                // phi[n * model->num_topics + k] = exp(phi[n * model->num_topics + k] - phisum);
                __m256fp ph_diff_sum = _mm256_sub(ph, ph_sum);
                ph = _mm256_exp(ph_diff_sum);
                _mm256_storeu(phi + (n * model->num_topics + k), ph);

                // var_gamma[k] = var_gamma[k] + doc->counts[n]*(phi[n * model->num_topics + k] - oldphi[k]);
                __m256fp ph_diff_old = _mm256_sub(ph, pho);
                vg = _mm256_fmadd(doc_counts, ph_diff_old, vg);
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
                __m256fp pho = _mm256_maskload(oldphi0 + k, rem);
                __m256fp vg = _mm256_maskload(var_gamma + k, rem);

                // phi[n * model->num_topics + k] = exp(phi[n * model->num_topics + k] - phisum);
                __m256fp ph_diff_sum = _mm256_sub(ph, ph_sum);
                ph = _mm256_exp(ph_diff_sum);
                _mm256_maskstore(phi + (n * model->num_topics + k), rem, ph);

                // var_gamma[k] = var_gamma[k] + doc->counts[n]*(phi[n * model->num_topics + k] - oldphi[k]);
                __m256fp ph_diff_old = _mm256_sub(ph, pho);
                vg = _mm256_fmadd(doc_counts, ph_diff_old, vg);
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
    fp_t likelihood = 0, digsum = 0, var_gamma_sum = 0;
    fp_t dig[model->num_topics];

    int k, n;

    int kk;
    __m256i leftover_mask;

    timer rdtsc = start_timer(LIKELIHOOD);

    STRIDE_SPLIT(model->num_topics, 0, &kk, &leftover_mask);
    int tiling_factor = 4;

    __m256fp v_var_gamma_sum0 = _mm256_set1(0);
    __m256fp v_var_gamma_sum1 = _mm256_set1(0);
    __m256fp v_var_gamma_sum2 = _mm256_set1(0);
    __m256fp v_var_gamma_sum3 = _mm256_set1(0);

    for (k = 0; k + (STRIDE * tiling_factor) - 1 < kk; k += STRIDE * tiling_factor)
    {
        //dig[k] = digamma(var_gamma[k]);
        //var_gamma_sum += var_gamma[k];
        
        // Tile 0
       __m256fp v_var_gamma0 = _mm256_loadu(var_gamma + k + 0 * tiling_factor);
       __m256fp v_dig0 = digamma_vec(v_var_gamma0);
       _mm256_storeu(dig + k + 0 * tiling_factor, v_dig0);
       v_var_gamma_sum0 = _mm256_add(v_var_gamma_sum0, v_var_gamma0);

       // Tile 1
        __m256fp v_var_gamma1 = _mm256_loadu(var_gamma + k + 1 * tiling_factor);
       __m256fp v_dig1 = digamma_vec(v_var_gamma1);
       _mm256_storeu(dig + k + 1 * tiling_factor, v_dig1);
       v_var_gamma_sum1 = _mm256_add(v_var_gamma_sum1, v_var_gamma1);

       //Tile 2
        __m256fp v_var_gamma2 = _mm256_loadu(var_gamma + k + 2 * tiling_factor);
       __m256fp v_dig2 = digamma_vec(v_var_gamma2);
       _mm256_storeu(dig + k + 2 * tiling_factor, v_dig2);
       v_var_gamma_sum2 = _mm256_add(v_var_gamma_sum2, v_var_gamma2);

       // Tile3
        __m256fp v_var_gamma3 = _mm256_loadu(var_gamma + k + 3 * tiling_factor);
       __m256fp v_dig3 = digamma_vec(v_var_gamma3);
       _mm256_storeu(dig + k + 3 * tiling_factor, v_dig3);
       v_var_gamma_sum3 = _mm256_add(v_var_gamma_sum3, v_var_gamma3);
    }

    v_var_gamma_sum0 = _mm256_add(v_var_gamma_sum0, v_var_gamma_sum1);
    v_var_gamma_sum0 = _mm256_add(v_var_gamma_sum0, v_var_gamma_sum2);
    v_var_gamma_sum0 = _mm256_add(v_var_gamma_sum0, v_var_gamma_sum3);

    for(;k < kk;k += STRIDE)
    {
        __m256fp v_var_gamma0 = _mm256_loadu(var_gamma + k);
        __m256fp v_dig0 = digamma_vec(v_var_gamma0);
        _mm256_storeu(dig + k, v_dig0);
        v_var_gamma_sum0 = _mm256_add(v_var_gamma_sum0, v_var_gamma0);
    }

    if (LEFTOVER(model->num_topics, 0)) {
       
       __m256fp v_var_gamma = _mm256_maskload(var_gamma + k, leftover_mask);
       __m256fp v_dig = digamma_vec(v_var_gamma);
       _mm256_maskstore(dig + k, leftover_mask, v_dig);
       v_var_gamma_sum0 = _mm256_add(v_var_gamma_sum0, v_var_gamma);
    }

    v_var_gamma_sum0 = hsum(v_var_gamma_sum0);
    var_gamma_sum = first(v_var_gamma_sum0);
    
    digsum = digamma(var_gamma_sum);



    __m256fp v_digsum = _mm256_set1(digsum);
    for (k = 0; k < kk; k += STRIDE) {
      //dig[k] = dig[k] - digsum;
      __m256fp v_dig = _mm256_loadu(dig + k);
      v_dig = _mm256_sub(v_dig, v_digsum);
      _mm256_storeu(dig + k, v_dig);
    }
    if (LEFTOVER(model->num_topics, 0)) {
      //dig[k] = dig[k] - digsum;
      __m256fp v_dig = _mm256_maskload(dig + k, leftover_mask);
      v_dig = _mm256_sub(v_dig, v_digsum);
      _mm256_maskstore(dig + k, leftover_mask, v_dig);
    }


    // <BG>: lgamma is a math library function
    likelihood = lgamma(model->alpha * model -> num_topics)
                - model -> num_topics * lgamma(model->alpha)
                - (lgamma(var_gamma_sum));
    
    // <SS>: TODO vectorized lgamma can be given entire array 
    // Compute the log likelihood dependent on the variational parameters
    // as per equation (15).
    
    fp_t alpha_m_1 = model->alpha - 1;
    __m256fp v_alpha_m_1 = _mm256_set1(alpha_m_1);
    __m256fp v_ones = _mm256_set1(1);
    fp_t log_gamma_result0[STRIDE]; // array of length 4
    fp_t log_gamma_result1[STRIDE];
    fp_t log_gamma_result2[STRIDE];
    fp_t log_gamma_result3[STRIDE];

    __m256fp v_likelihood_k0 = _mm256_set1(0);
    __m256fp v_likelihood_k1 = _mm256_set1(0);
    __m256fp v_likelihood_k2 = _mm256_set1(0);
    __m256fp v_likelihood_k3 = _mm256_set1(0);


    for (k = 0; k + (STRIDE * tiling_factor) - 1 < kk; k += STRIDE * tiling_factor)
    {
        // likelihood += (model->alpha - 1)*dig[k]
        //             + lgamma(var_gamma[k])
        //             - (var_gamma[k] - 1)*dig[k];
        
        // Tile 0
        vdLGamma(STRIDE, var_gamma + k + 0 * tiling_factor, log_gamma_result0);            
        __m256fp v_lgamma0 = _mm256_loadu(log_gamma_result0);

        __m256fp v_dig0 = _mm256_loadu(dig + k + 0 * tiling_factor);
        __m256fp v_t00 = _mm256_mul(v_dig0, v_alpha_m_1);

        __m256fp v_var_gamma0 = _mm256_loadu(var_gamma + k + 0 * tiling_factor);
        v_var_gamma0 = _mm256_sub(v_var_gamma0, v_ones);
        v_dig0 = _mm256_mul(v_var_gamma0, v_dig0);

        v_likelihood_k0 = _mm256_add(v_likelihood_k0, v_lgamma0);
        v_likelihood_k0 = _mm256_sub(v_likelihood_k0, v_dig0);
        v_likelihood_k0 = _mm256_add(v_likelihood_k0, v_t00);

        // Tile 1
        vdLGamma(STRIDE, var_gamma + k + 1 * tiling_factor, log_gamma_result1); 
        __m256fp v_lgamma1 = _mm256_loadu(log_gamma_result1);

        __m256fp v_dig1 = _mm256_loadu(dig + k + 1 * tiling_factor);
        __m256fp v_t01 = _mm256_mul(v_dig1, v_alpha_m_1);

        __m256fp v_var_gamma1 = _mm256_loadu(var_gamma + k + 1 * tiling_factor);
        v_var_gamma1 = _mm256_sub(v_var_gamma1, v_ones);
        v_dig1 = _mm256_mul(v_var_gamma1, v_dig1);

        v_likelihood_k1 = _mm256_add(v_likelihood_k1, v_lgamma1);
        v_likelihood_k1 = _mm256_sub(v_likelihood_k1, v_dig1);
        v_likelihood_k1 = _mm256_add(v_likelihood_k1, v_t01);

        // Tile 2
        vdLGamma(STRIDE, var_gamma + k + 2 * tiling_factor, log_gamma_result2); 
        __m256fp v_lgamma2 = _mm256_loadu(log_gamma_result2);

        __m256fp v_dig2 = _mm256_loadu(dig + k + 2 * tiling_factor);
        __m256fp v_t02 = _mm256_mul(v_dig2, v_alpha_m_1);

        __m256fp v_var_gamma2 = _mm256_loadu(var_gamma + k + 2 * tiling_factor);
        v_var_gamma2 = _mm256_sub(v_var_gamma2, v_ones);
        v_dig2 = _mm256_mul(v_var_gamma2, v_dig2);

        v_likelihood_k2 = _mm256_add(v_likelihood_k2, v_lgamma2);
        v_likelihood_k2 = _mm256_sub(v_likelihood_k2, v_dig2);
        v_likelihood_k2 = _mm256_add(v_likelihood_k2, v_t02);

        // Tile 3
        vdLGamma(STRIDE, var_gamma + k + 3 * tiling_factor, log_gamma_result3); 
        __m256fp v_lgamma3 = _mm256_loadu(log_gamma_result3);

        __m256fp v_dig3 = _mm256_loadu(dig + k + 3 * tiling_factor);
        __m256fp v_t03 = _mm256_mul(v_dig3, v_alpha_m_1);

        __m256fp v_var_gamma3 = _mm256_loadu(var_gamma + k + 3 * tiling_factor);
        v_var_gamma3 = _mm256_sub(v_var_gamma3, v_ones);
        v_dig3 = _mm256_mul(v_var_gamma3, v_dig3);

        v_likelihood_k3 = _mm256_add(v_likelihood_k3, v_lgamma3);
        v_likelihood_k3 = _mm256_sub(v_likelihood_k3, v_dig3);
        v_likelihood_k3 = _mm256_add(v_likelihood_k3, v_t03);
                  
    }
    v_likelihood_k0 = _mm256_add(v_likelihood_k0, v_likelihood_k1);
    v_likelihood_k0 = _mm256_add(v_likelihood_k0, v_likelihood_k2);
    v_likelihood_k0 = _mm256_add(v_likelihood_k0, v_likelihood_k3);

    for(;k < kk; k += STRIDE)
    {
        // still vectorized, but not tiled
        vdLGamma(STRIDE, var_gamma + k, log_gamma_result0);
        __m256fp v_lgamma0 = _mm256_loadu(log_gamma_result0);

        __m256fp v_dig0 = _mm256_loadu(dig + k);
        __m256fp v_t00 = _mm256_mul(v_dig0, v_alpha_m_1);

        __m256fp v_var_gamma0 = _mm256_loadu(var_gamma + k);
        v_var_gamma0 = _mm256_sub(v_var_gamma0, v_ones);
        v_dig0 = _mm256_mul(v_var_gamma0, v_dig0);

        v_likelihood_k0 = _mm256_add(v_likelihood_k0, v_lgamma0);
        v_likelihood_k0 = _mm256_sub(v_likelihood_k0, v_dig0);
        v_likelihood_k0 = _mm256_add(v_likelihood_k0, v_t00);
        
    }
    if (LEFTOVER(model->num_topics, 0)) {

        vdLGamma(LEFTOVER(model->num_topics, 0), var_gamma + k, log_gamma_result0);
        __m256fp v_lgamma = _mm256_maskload(log_gamma_result0, leftover_mask);

        __m256fp v_dig = _mm256_maskload(dig + k, leftover_mask);
        __m256fp v_t0 = _mm256_mul(v_dig, v_alpha_m_1);

        __m256fp v_var_gamma = _mm256_maskload(var_gamma + k, leftover_mask);
        v_var_gamma = _mm256_sub(v_var_gamma, v_ones);
        v_dig = _mm256_mul(v_var_gamma, v_dig);


        v_likelihood_k0 = _mm256_add(v_likelihood_k0, v_lgamma);
        v_likelihood_k0 = _mm256_sub(v_likelihood_k0, v_dig);
        v_likelihood_k0 = _mm256_add(v_likelihood_k0, v_t0);
    }



    v_likelihood_k0 = hsum(v_likelihood_k0);

    // <BG> += because there are multiple places where stuff gets added up to likelihood
    likelihood += first(v_likelihood_k0);


    __m256fp v_likelihood0 = _mm256_set1(0);
    __m256fp v_likelihood1 = _mm256_set1(0);
    __m256fp v_likelihood2 = _mm256_set1(0);
    __m256fp v_likelihood3 = _mm256_set1(0);

    __m256fp LOW = _mm256_set1(-100);

    // <CC> Swapped loop to have the strided access to the transposed
    // 
    // printf("%d\n", doc->length);
    for (n = 0; n + tiling_factor - 1 < doc->length; n += tiling_factor)
    {
        __m256fp v_doc_counts0 = _mm256_set1(doc->counts[n+0]);
        __m256fp v_doc_counts1 = _mm256_set1(doc->counts[n+1]);
        __m256fp v_doc_counts2 = _mm256_set1(doc->counts[n+2]);
        __m256fp v_doc_counts3 = _mm256_set1(doc->counts[n+3]);

        for (k = 0; k < kk; k += STRIDE)
        {


            // t1 = dig[k];
            // t2 = phi[n * model->num_topics + k];
            // t3 = log(t2)
            // t3 = MAX(t3, -100);
            // t4 = model->log_prob_w_doc[n * model->num_topics + k];
            // t5 = (t1 - t3 + t4);
            // t6 = doc->counts[n] * ( t2 *  t5);
            // likelihood = likelihood + t6;


            __m256fp v_dig = _mm256_loadu(dig + k);

            // Tile 0
            __m256fp v_phi0 = _mm256_loadu(phi + (n+0) * model->num_topics + k);
            __m256fp v_log_phi0 = _mm256_log(v_phi0); 
            v_log_phi0 = _mm256_max(v_log_phi0, LOW);
            __m256fp v_l_p_w_doc0 = _mm256_loadu(model->log_prob_w_doc + (n+0) * model->num_topics + k);
            __m256fp v_t50 = _mm256_add(_mm256_sub(v_dig, v_log_phi0), v_l_p_w_doc0);
            __m256fp v_doc_likelihood0 = _mm256_mul(v_doc_counts0, _mm256_mul(v_phi0, v_t50));

            v_likelihood0 = _mm256_add(v_likelihood0, v_doc_likelihood0);

            // Tile 1
            __m256fp v_phi1 = _mm256_loadu(phi + (n+1) * model->num_topics + k);
            __m256fp v_log_phi1 = _mm256_log(v_phi1); 
            v_log_phi1 = _mm256_max(v_log_phi1, LOW);
            __m256fp v_l_p_w_doc1 = _mm256_loadu(model->log_prob_w_doc + (n+1) * model->num_topics + k);
            __m256fp v_t51 = _mm256_add(_mm256_sub(v_dig, v_log_phi1), v_l_p_w_doc1);
            __m256fp v_doc_likelihood1 = _mm256_mul(v_doc_counts1, _mm256_mul(v_phi1, v_t51));

            v_likelihood1 = _mm256_add(v_likelihood1, v_doc_likelihood1);

            // Tile 2
            __m256fp v_phi2 = _mm256_loadu(phi + (n+2) * model->num_topics + k);
            __m256fp v_log_phi2 = _mm256_log(v_phi2); 
            v_log_phi2 = _mm256_max(v_log_phi2, LOW);
            __m256fp v_l_p_w_doc2 = _mm256_loadu(model->log_prob_w_doc + (n+2) * model->num_topics + k);
            __m256fp v_t52 = _mm256_add(_mm256_sub(v_dig, v_log_phi2), v_l_p_w_doc2);
            __m256fp v_doc_likelihood2 = _mm256_mul(v_doc_counts2, _mm256_mul(v_phi2, v_t52));

            v_likelihood2 = _mm256_add(v_likelihood2, v_doc_likelihood2);

            // Tile 3
            __m256fp v_phi3 = _mm256_loadu(phi + (n+3) * model->num_topics + k);
            __m256fp v_log_phi3 = _mm256_log(v_phi3); 
            v_log_phi3 = _mm256_max(v_log_phi3, LOW);
            __m256fp v_l_p_w_doc3 = _mm256_loadu(model->log_prob_w_doc + (n+3) * model->num_topics + k);
            __m256fp v_t53 = _mm256_add(_mm256_sub(v_dig, v_log_phi3), v_l_p_w_doc3);
            __m256fp v_doc_likelihood3 = _mm256_mul(v_doc_counts3, _mm256_mul(v_phi3, v_t53));

            v_likelihood3 = _mm256_add(v_likelihood3, v_doc_likelihood3);
        }
        if (LEFTOVER(model->num_topics, 0)) {
            __m256fp v_dig = _mm256_maskload(dig + k, leftover_mask);

            // Tile 0
            __m256fp v_phi0 = _mm256_maskload(phi + (n+0) * model->num_topics + k, leftover_mask);
            __m256fp v_log_phi0 = _mm256_log(v_phi0);
            v_log_phi0 = _mm256_max(v_log_phi0, LOW);
            __m256fp v_l_p_w_doc0 = _mm256_maskload(model->log_prob_w_doc + (n+0) * model->num_topics + k, leftover_mask);
            __m256fp v_t50 = _mm256_add(_mm256_sub(v_dig, v_log_phi0), v_l_p_w_doc0);
            __m256fp v_doc_likelihood0 = _mm256_mul(v_doc_counts0, _mm256_mul(v_phi0, v_t50));

            v_likelihood0 = _mm256_add(v_likelihood0, v_doc_likelihood0);

            // Tile 1
            __m256fp v_phi1 = _mm256_maskload(phi + (n+1) * model->num_topics + k, leftover_mask);
            __m256fp v_log_phi1 = _mm256_log(v_phi1);
            v_log_phi1 = _mm256_max(v_log_phi1, LOW);
            __m256fp v_l_p_w_doc1 = _mm256_maskload(model->log_prob_w_doc + (n+1) * model->num_topics + k, leftover_mask);
            __m256fp v_t51 = _mm256_add(_mm256_sub(v_dig, v_log_phi1), v_l_p_w_doc1);
            __m256fp v_doc_likelihood1 = _mm256_mul(v_doc_counts1, _mm256_mul(v_phi1, v_t51));

            v_likelihood1 = _mm256_add(v_likelihood1, v_doc_likelihood1);


            // Tile 2
            __m256fp v_phi2 = _mm256_maskload(phi + (n+2) * model->num_topics + k, leftover_mask);
            __m256fp v_log_phi2 = _mm256_log(v_phi2);
            v_log_phi2 = _mm256_max(v_log_phi2, LOW);
            __m256fp v_l_p_w_doc2 = _mm256_maskload(model->log_prob_w_doc + (n+2) * model->num_topics + k, leftover_mask);
            __m256fp v_t52 = _mm256_add(_mm256_sub(v_dig, v_log_phi2), v_l_p_w_doc2);
            __m256fp v_doc_likelihood2 = _mm256_mul(v_doc_counts2, _mm256_mul(v_phi2, v_t52));

            v_likelihood2 = _mm256_add(v_likelihood2, v_doc_likelihood2);

            // Tile 3
            __m256fp v_phi3 = _mm256_maskload(phi + (n+3) * model->num_topics + k, leftover_mask);
            __m256fp v_log_phi3 = _mm256_log(v_phi3);
            v_log_phi3 = _mm256_max(v_log_phi3, LOW);
            __m256fp v_l_p_w_doc3 = _mm256_maskload(model->log_prob_w_doc + (n+3) * model->num_topics + k, leftover_mask);
            __m256fp v_t53 = _mm256_add(_mm256_sub(v_dig, v_log_phi3), v_l_p_w_doc3);
            __m256fp v_doc_likelihood3 = _mm256_mul(v_doc_counts3, _mm256_mul(v_phi3, v_t53));

            v_likelihood3 = _mm256_add(v_likelihood3, v_doc_likelihood3);
        }
    }

    // add the four likelihood vectors up so they don't stick around in register
    v_likelihood0 = _mm256_add(v_likelihood0, v_likelihood1);
    v_likelihood0 = _mm256_add(v_likelihood0, v_likelihood2);
    v_likelihood0 = _mm256_add(v_likelihood0, v_likelihood3);

    for (; n < doc->length; n++)
    {
        __m256fp v_doc_counts0 = _mm256_set1(doc->counts[n]);
        for (k = 0; k < kk; k += STRIDE)
        {   
            __m256fp v_dig = _mm256_loadu(dig + k);

            // Add to likelihood0
            __m256fp v_phi0 = _mm256_loadu(phi + (n+0) * model->num_topics + k);
            __m256fp v_log_phi0 = _mm256_log(v_phi0);
            v_log_phi0 = _mm256_max(v_log_phi0, LOW);
            __m256fp v_l_p_w_doc0 = _mm256_loadu(model->log_prob_w_doc + (n+0) * model->num_topics + k);
            __m256fp v_t50 = _mm256_add(_mm256_sub(v_dig, v_log_phi0), v_l_p_w_doc0);
            __m256fp v_doc_likelihood0 = _mm256_mul(v_doc_counts0, _mm256_mul(v_phi0, v_t50));

            v_likelihood0 = _mm256_add(v_likelihood0, v_doc_likelihood0);
        }
        if (LEFTOVER(model->num_topics, 0)) {
            __m256fp v_dig = _mm256_maskload(dig + k, leftover_mask);

            // Add to likelihood0
            __m256fp v_phi0 = _mm256_maskload(phi + (n+0) * model->num_topics + k, leftover_mask);
            __m256fp v_log_phi0 = _mm256_log(v_phi0);
            v_log_phi0 = _mm256_max(v_log_phi0, LOW);
            __m256fp v_l_p_w_doc0 = _mm256_maskload(model->log_prob_w_doc + (n+0) * model->num_topics + k, leftover_mask);
            __m256fp v_t50 = _mm256_add(_mm256_sub(v_dig, v_log_phi0), v_l_p_w_doc0);
            __m256fp v_doc_likelihood0 = _mm256_mul(v_doc_counts0, _mm256_mul(v_phi0, v_t50));

            v_likelihood0 = _mm256_add(v_likelihood0, v_doc_likelihood0);
        }

    } 

    __m256fp likelihood_totals = hsum(v_likelihood0);
    // NOTE += and not = because some part of likelihood is scalar computed
    likelihood += first(likelihood_totals);
    // printf("%f\n", likelihood);

    stop_timer(rdtsc);

    return likelihood;
}