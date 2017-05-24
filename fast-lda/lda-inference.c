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
    double var_gamma_init_value = model->alpha + (doc->total/((fp_t) model->num_topics));

    __m256fp vg = _mm256_set1(var_gamma_init_value);

    for (k = 0; k < kk; k += STRIDE)
    {
        __m256fp dg = digamma_vec(vg);

        _mm256_storeu(var_gamma + k, vg);
        _mm256_storeu(digamma_gam + k, dg);
    }    

    if (LEFTOVER(model->num_topics, 0))
    {
        __m256fp dg = digamma_vec(vg);

        _mm256_maskstore(var_gamma + k, rem, vg);
        _mm256_maskstore(digamma_gam + k, rem, dg);
    }

    __m256fp ph_init_value = _mm256_set1(1.0/model->num_topics);

    for (n = 0; n < doc->length; n++)
    {
        for (k = 0; k < kk; k += STRIDE)
        {
            _mm256_storeu(phi + (n * model->num_topics + k), ph_init_value);
        }
        if (LEFTOVER(model->num_topics, 0))
        {
            _mm256_maskstore(phi + (n * model->num_topics + kk), rem, ph_init_value);
        }
    }


    var_iter = 0;

    while ((converged > VAR_CONVERGED) &&
     ((var_iter < VAR_MAX_ITER) || (VAR_MAX_ITER == -1)))
    {
       var_iter++;
       // Update equation (16) for variational phi for each topic and word.
       for (n = 0; n < doc->length; n++)
       {
            // <BG, SS> Moved if else initialization of phisum outside of the loop

            oldphi[0] = phi[n * model->num_topics + 0];
            phi[n * model->num_topics + 0] = digamma_gam[0] + model->log_prob_w[doc->words[n] * model->num_topics + 0];
            phisum = phi[n * model->num_topics + 0];

            int kk1;
            __m256i rem1;
            STRIDE_SPLIT(model->num_topics, 1, &kk1, &rem1);

            for (k = 1; k < kk1; k += STRIDE)
            {
                //oldphi[k] = phi[n * model->num_topics + k];
                __m256fp ph = _mm256_loadu(phi + (n * model->num_topics + k));
                _mm256_storeu(oldphi + k, ph);

                //phi[n * model->num_topics + k] = digamma_gam[k] + model->log_prob_w_doc[n * model->num_topics + k];
                __m256fp dg = _mm256_loadu(digamma_gam + k);
                __m256fp lpwd = _mm256_loadu(model->log_prob_w_doc + (n * model->num_topics + k));

                ph = _mm256_add(dg, lpwd);
                _mm256_storeu(phi + (n * model->num_topics + k), ph);
            }  

            if (LEFTOVER(model->num_topics, 1)) {
                __m256fp ph = _mm256_maskload(phi + (n * model->num_topics + k), rem1);
                _mm256_maskstore(oldphi + k, rem1, ph);

                __m256fp dg = _mm256_maskload(digamma_gam + k, rem1);
                __m256fp lpwd = _mm256_maskload(model->log_prob_w_doc + (n * model->num_topics + k), rem1);

                ph = _mm256_add(dg, lpwd);
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
    fp_t likelihood = 0, digsum = 0, var_gamma_sum = 0;
    fp_t dig[model->num_topics];
    __m256fp v_likelihood = _mm256_set1(0), 
             v_var_gamma_sum = _mm256_set1(0);
    int k, n;

    int kk;
    __m256i leftover_mask;

    timer rdtsc = start_timer(LIKELIHOOD);

    STRIDE_SPLIT(model->num_topics, 0, &kk, &leftover_mask);

    for (k = 0; k < kk; k += STRIDE)
    {
       //dig[k] = digamma(var_gamma[k]);
       __m256fp v_var_gamma = _mm256_loadu(var_gamma + k);
       __m256fp v_dig = digamma_vec(v_var_gamma);
       _mm256_storeu(dig + k, v_dig);

       //var_gamma_sum += var_gamma[k];
       v_var_gamma_sum = _mm256_add(v_var_gamma_sum, v_var_gamma);
    }

    if (LEFTOVER(model->num_topics, 0)) {
       //dig[k] = digamma(var_gamma[k]);
       __m256fp v_var_gamma = _mm256_maskload(var_gamma + k, leftover_mask);
       __m256fp v_dig = digamma_vec(v_var_gamma);
       _mm256_maskstore(dig + k, leftover_mask, v_dig);

       //var_gamma_sum += var_gamma[k];
       v_var_gamma_sum = _mm256_add(v_var_gamma_sum, v_var_gamma);
    }

    __m256fp var_gamma_totals = hsum(v_var_gamma_sum);
    var_gamma_sum = first(var_gamma_totals);
    
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
    
    __m256fp v_likelihood_2 = _mm256_set1(0);
    fp_t alpha_m_1 = model->alpha - 1;
    __m256fp v_alpha_m_1 = _mm256_set1(alpha_m_1);
    __m256fp v_ones = _mm256_set1(1);
    fp_t log_gamma_result[STRIDE];
    for (k = 0; k < kk; k += STRIDE)
    {
        // likelihood += (model->alpha - 1)*dig[k]
        //             + lgamma(var_gamma[k])
        //             - (var_gamma[k] - 1)*dig[k];
        vdLGamma(STRIDE, var_gamma + k, log_gamma_result);
        __m256fp v_lgamma = _mm256_loadu(log_gamma_result);

        __m256fp v_dig = _mm256_loadu(dig + k);
        __m256fp v_t0 = _mm256_mul(v_dig, v_alpha_m_1);

        __m256fp v_var_gamma = _mm256_loadu(var_gamma + k);
        v_var_gamma = _mm256_sub(v_var_gamma, v_ones);
        v_dig = _mm256_mul(v_var_gamma, v_dig);


        v_likelihood_2 = _mm256_add(v_likelihood_2, v_lgamma);
        v_likelihood_2 = _mm256_sub(v_likelihood_2, v_dig);

        v_likelihood_2 = _mm256_add(v_likelihood_2, v_t0);
                  
    }
    if (LEFTOVER(model->num_topics, 0)) {
        vdLGamma(LEFTOVER(model->num_topics, 0), var_gamma + k, log_gamma_result);
        __m256fp v_lgamma = _mm256_maskload(log_gamma_result, leftover_mask);

        __m256fp v_dig = _mm256_maskload(dig + k, leftover_mask);
        __m256fp v_t0 = _mm256_mul(v_dig, v_alpha_m_1);

        __m256fp v_var_gamma = _mm256_maskload(var_gamma + k, leftover_mask);
        v_var_gamma = _mm256_sub(v_var_gamma, v_ones);
        v_dig = _mm256_mul(v_var_gamma, v_dig);


        v_likelihood_2 = _mm256_add(v_likelihood_2, v_lgamma);
        v_likelihood_2 = _mm256_sub(v_likelihood_2, v_dig);

        v_likelihood_2 = _mm256_add(v_likelihood_2, v_t0);
    }
    v_likelihood_2 = hsum(v_likelihood_2);
    likelihood += first(v_likelihood_2);



    fp_t logcheck;
    // <CC> Swapped loop to have the strided access to the transposed
    for (n = 0; n < doc->length; n++)
    {
        __m256fp v_doc_counts = _mm256_set1(doc->counts[n]); 

        for (k = 0; k < kk; k += STRIDE)
        {
            //t1 = dig[k];
            __m256fp v_dig = _mm256_loadu(dig + k);

            // t2 = phi[n * model->num_topics + k];
            __m256fp v_phi = _mm256_loadu(phi + n * model->num_topics + k);

            // t3 = log(t2)
            __m256fp v_log_phi = _mm256_log(v_phi); 

            // <SS> seems like this can be removed, the original code
            // has nothing going into else condition
            // t3 = MAX(t3, -100);
            __m256fp LOW = _mm256_set1(-100);
            v_log_phi = _mm256_max(v_log_phi, LOW);

            // t4 = model->log_prob_w_doc[n * model->num_topics + k];
            // <SS> variable names are going out of hand -.-
            __m256fp v_l_p_w_doc = _mm256_loadu(model->log_prob_w_doc + n * model->num_topics + k);

            // t5 = (t1 - t3 + t4);
            __m256fp v_t5 = _mm256_add(_mm256_sub(v_dig, v_log_phi), v_l_p_w_doc);

            // t6 = doc->counts[n] * ( t2 *  t5);
            __m256fp v_doc_likelihood = _mm256_mul(v_doc_counts, _mm256_mul(v_phi, v_t5));

            // likelihood = likelihood + t6;
            v_likelihood = _mm256_add(v_likelihood, v_doc_likelihood);
        }
        if (LEFTOVER(model->num_topics, 0)) {
            //t1 = dig[k];
            __m256fp v_dig = _mm256_maskload(dig + k, leftover_mask);

            // t2 = phi[n * model->num_topics + k];
            __m256fp v_phi = _mm256_maskload(phi + n * model->num_topics + k, leftover_mask);

            // t3 = log(t2)
            __m256fp v_log_phi = _mm256_log(v_phi); 

            // <SS> seems like this can be removed, the original code
            // has nothing going into else condition
            // t3 = MAX(t3, -100);
            __m256fp LOW = _mm256_set1(-100);
            v_log_phi = _mm256_max(v_log_phi, LOW);

            // t4 = model->log_prob_w_doc[n * model->num_topics + k];
            // <SS> variable names are going out of hand -.-
            __m256fp v_l_p_w_doc = _mm256_maskload(model->log_prob_w_doc + n * model->num_topics + k, leftover_mask);

            // t5 = (t1 - t3 + t4);
            __m256fp v_t5 = _mm256_add(_mm256_sub(v_dig, v_log_phi), v_l_p_w_doc);

            // t6 = doc->counts[n] * ( t2 *  t5);
            __m256fp v_doc_likelihood = _mm256_mul(v_doc_counts, _mm256_mul(v_phi, v_t5));

            // likelihood = likelihood + t6;
            v_likelihood = _mm256_add(v_likelihood, v_doc_likelihood);
        }
    }

    __m256fp likelihood_totals = hsum(v_likelihood);
    // NOTE += and not = because some part of likelihood is scalar computed
    likelihood += first(likelihood_totals);

    stop_timer(rdtsc);

    return likelihood;
}
