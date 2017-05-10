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

#include "lda-alpha.h"
#include "rdtsc-helper.h"

/*
 * objective function and its derivatives
 *
 */

fp_t alhood(fp_t a, fp_t ss, int D, int K)
{ return(D * (lgamma(K * a) - K * lgamma(a)) + (a - 1) * ss); }

fp_t d_alhood(fp_t a, fp_t ss, int D, int K)
{ return(D * (K * digamma(K * a) - K * digamma(a)) + ss); }

fp_t d2_alhood(fp_t a, int D, int K)
{ return(D * (K * K * trigamma(K * a) - K * trigamma(a))); }


/*
 * newtons method
 *
 */

fp_t opt_alpha(fp_t ss, int D, int K)
{
    fp_t a, log_a, init_a = 100;
    fp_t f, df, d2f;
    int iter = 0;

    log_a = log(init_a);
    do
    {
        iter++;
        a = exp(log_a);
        if (isnan(a))
        {
            init_a = init_a * 10;
            printf("warning : alpha is nan; new init = %5.5f\n", init_a);
            a = init_a;
            log_a = log(a);
        }
        f = alhood(a, ss, D, K);
        df = d_alhood(a, ss, D, K);
        d2f = d2_alhood(a, D, K);
        log_a = log_a - df/(d2f * a + df);
        printf("alpha maximization : %5.5f   %5.5f\n", f, df);
    }
    while ((fabs(df) > NEWTON_THRESH) && (iter < MAX_ALPHA_ITER));
    return(exp(log_a));
}
