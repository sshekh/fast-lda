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
#include "rdtsc-helper.h"

fp_t lda_inference(document* doc, lda_model* model, fp_t* var_gamma, fp_t* phi)
{
    fp_t converged = 1;
    fp_t phisum = 0, likelihood = 0;
    fp_t likelihood_old = 0, oldphi[model->num_topics];
    int k, n, var_iter;
    fp_t digamma_gam[model->num_topics];

    timer rdtsc = start_timer(LDA_INFERENCE);

    // Initialize the phi for all topics and all words in the doc
    // and compute digamma of the sum of variational gammas over all the topics.
    for (k = 0; k < model->num_topics; k++)
    {
        var_gamma[k] = model->alpha + (doc->total/((fp_t) model->num_topics));
        digamma_gam[k] = digamma(var_gamma[k]);
    }
    for (n = 0; n < doc->length; n++)
    {
        for (k = 0; k < model->num_topics; k++)
        {
            phi[n * model->num_topics + k] = 1.0/model->num_topics;
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
            for (k = 1; k < model->num_topics; k++)
            {
                oldphi[k] = phi[n * model->num_topics + k];
                // Eq (16)

                phi[n * model->num_topics + k] = digamma_gam[k] + model->log_prob_w_doc[n * model->num_topics + k];

                phisum = log_sum(phisum, phi[n * model->num_topics + k]);
            }

            //Update equation (17) for variational gamma for each topic
            for (k = 0; k < model->num_topics; k++)
            {
                // Write the final value of the update for phi.
                phi[n * model->num_topics + k] = exp(phi[n * model->num_topics + k] - phisum);
                var_gamma[k] = var_gamma[k] + doc->counts[n]*(phi[n * model->num_topics + k] - oldphi[k]);
                // !!! a lot of extra digamma's here because of how we're computing it
                // !!! but its more automatically updated too.
                digamma_gam[k] = digamma(var_gamma[k]);
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
