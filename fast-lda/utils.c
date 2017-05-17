#include "utils.h"
#include "rdtsc-helper.h"

/*
 * given log(a) and log(b), return log(a + b)
 *
 */

fp_t log_sum(fp_t log_a, fp_t log_b)
{
  timer rdtsc = start_timer(LOG_SUM);

  fp_t v;
  if (log_a < log_b)
  {
      v = log_b+log(1 + exp(log_a-log_b));
  }
  else
  {
      v = log_a+log(1 + exp(log_b-log_a));
  }

  stop_timer(rdtsc);
  return(v);
}

 /**
   * Proc to calculate the value of the trigamma, the second
   * derivative of the loggamma function. Accepts positive matrices.
   * From Abromowitz and Stegun.  Uses formulas 6.4.11 and 6.4.12 with
   * recurrence formula 6.4.6.  Each requires workspace at least 5
   * times the size of X.
   *
   **/

fp_t trigamma(fp_t x)
{
    fp_t p;
    int i;

    timer t = start_timer(TRIGAMMA);

    x=x+6;
    p=1/(x*x);
    p=(((((0.075757575757576*p-0.033333333333333)*p+0.0238095238095238)
         *p-0.033333333333333)*p+0.166666666666667)*p+1)/x+0.5*p;
    for (i=0; i<6 ;i++)
    {
        x=x-1;
        p=1/(x*x)+p;
    }

    stop_timer(t);

    return(p);
}


/*
 * taylor approximation of first derivative of the log gamma function
 *
 */

fp_t digamma(fp_t x)
{
    timer rdtsc = start_timer(DIGAMMA);

    fp_t p;
    x=x+6;
    p=1/(x*x);
    p=(((0.004166666666667*p-0.003968253986254)*p+
	 0.008333333333333)*p-0.083333333333333)*p;
    p=p+log(x)-0.5/x-1/(x-1)-1/(x-2)-1/(x-3)-1/(x-4)-1/(x-5)-1/(x-6);

    stop_timer(rdtsc);
    return p;
}

__m256fp digamma_vec(__m256fp x)
{
    timer rdtsc = start_timer(DIGAMMA);


    __m256fp HALVES = _mm256_set1(0.5);
    __m256fp ONES = _mm256_set1(1);
    __m256fp TWOS = _mm256_set1(2);
    __m256fp THREES = _mm256_set1(3);
    __m256fp FOURS = _mm256_set1(4);
    __m256fp FIVES = _mm256_set1(5);
    __m256fp SIXES = _mm256_set1(6);
    __m256fp ONE_120TH = _mm256_set1((fp_t) 0.008333333333333);
    __m256fp ONE_240TH = _mm256_set1((fp_t) 0.004166666666667);
    // <FL> I have no idea what this is. Google returns other implementations of
    // the digamma function or things like "the epic floating point battle".
    __m256fp DIGAMMA_CONST = _mm256_set1((fp_t) 0.003968253986254);

    // x = x + 6
    __m256fp x6  = _mm256_add(x, SIXES);

    // p = 1 / (x*x)
    __m256fp xsq = _mm256_mul(x6, x6);
    __m256fp p   = _mm256_rcp(xsq);

    /* <FL>
     * Use fp associativity.
     * We go from (((p/240 - const)p + 1/120)p - 1/240)p
     * to (p/240 - const) p^3 + p^2/120 - p/240
     * This strikes a balance between making the tree shallower and increasing
     * the number of flops. The first version takes 28 cycles, while this one
     * takes 20.
     */
    __m256fp p240 = _mm256_mul(p, ONE_240TH);
    __m256fp psq = _mm256_mul(p, p);

    __m256fp psq120 = _mm256_mul(psq, ONE_120TH);
    __m256fp pcu = _mm256_mul(psq, p);
    __m256fp p240c = _mm256_sub(p240, DIGAMMA_CONST);

    // No ILP here
    __m256fp r = _mm256_mul(p240c, pcu);
    __m256fp q = _mm256_add(r, psq120);
    p = _mm256_sub(q, p240);


    // p+log(x)-0.5/x-1/(x-1)-1/(x-2)-1/(x-3)-1/(x-4)-1/(x-5)-1/(x-6)
    // Tons of ILP here
    __m256fp logx = _mm256_log(x6);
    __m256fp hox = _rcp_const(HALVES, x6);

    __m256fp xm1 = _mm256_sub(x6, ONES);
    __m256fp xm2 = _mm256_sub(x6, TWOS);
    __m256fp xm3 = _mm256_sub(x6, THREES);
    __m256fp xm4 = _mm256_sub(x6, FOURS);
    __m256fp xm5 = _mm256_sub(x6, FIVES);
    __m256fp xm6 = _mm256_sub(x6, SIXES);

    __m256fp xm1r = _mm256_rcp(xm1);
    __m256fp xm2r = _mm256_rcp(xm2);
    __m256fp xm3r = _mm256_rcp(xm3);
    __m256fp xm4r = _mm256_rcp(xm4);
    __m256fp xm5r = _mm256_rcp(xm5);
    __m256fp xm6r = _mm256_rcp(xm6);

    // Maximally exploit associativity
    __m256fp logx_m_hox = _mm256_sub(logx, hox);
    __m256fp xm12r = _mm256_add(xm1r, xm2r);
    __m256fp xm34r = _mm256_add(xm3r, xm4r);
    __m256fp xm56r = _mm256_add(xm5r, xm6r);

    __m256fp a = _mm256_sub(logx_m_hox, xm12r);
    __m256fp b = _mm256_add(xm34r, xm56r);
    __m256fp c = _mm256_sub(a, b);
    __m256fp result = _mm256_add(p, c);

    stop_timer(rdtsc);
    return result;
}


fp_t log_gamma(fp_t x)
{
    timer rdtsc = start_timer(LOG_GAMMA);

     fp_t z=1/(x*x);

    x=x+6;
    z=(((-0.000595238095238*z+0.000793650793651)
	*z-0.002777777777778)*z+0.083333333333333)/x;
    z=(x-0.5)*log(x)-x+0.918938533204673+z-log(x-1)-
	log(x-2)-log(x-3)-log(x-4)-log(x-5)-log(x-6);

    stop_timer(rdtsc);
    return z;
}

__m256fp log_gamma_vec(__m256fp x)
{
    timer rdtsc = start_timer(LOG_GAMMA);

    __m256fp HALVES = _mm256_set1(0.5);
    __m256fp ONES = _mm256_set1(1);
    __m256fp TWOS = _mm256_set1(2);
    __m256fp THREES = _mm256_set1(3);
    __m256fp FOURS = _mm256_set1(4);
    __m256fp FIVES = _mm256_set1(5);
    __m256fp SIXES = _mm256_set1(6);
    __m256fp RM1680 = _mm256_set1(-0.000595238095238);
    __m256fp R1260 = _mm256_set1(0.000793650793651);
    __m256fp R360 = _mm256_set1(0.002777777777778);
    __m256fp R12 = _mm256_set1(0.083333333333333);
    //<FL> Another unexplainable constant
    __m256fp LGAM_CONST = _mm256_set1(0.918938533204673);

    // z = 1 / (x*x)
    __m256fp xsq = _mm256_mul(x, x);
    __m256fp z = _mm256_rcp(xsq);

    // x = x + 6
    x = _mm256_add(x, SIXES);

    // <FL> Sadly we cannot take advantage of fp associativity for this z update
    z = _mm256_mul(RM1680, z);
    z = _mm256_add(z, R1260);
    z = _mm256_mul(z, z);
    z = _mm256_sub(z, R360);
    z = _mm256_mul(z, z);
    z = _mm256_add(z, R12);
    z = _rcp_const(z, x);

    __m256fp xh = _mm256_sub(x, HALVES);
    __m256fp x1 = _mm256_sub(x, ONES);
    __m256fp x2 = _mm256_sub(x, TWOS);
    __m256fp x3 = _mm256_sub(x, THREES);
    __m256fp x4 = _mm256_sub(x, FOURS);
    __m256fp x5 = _mm256_sub(x, FIVES);
    __m256fp x6 = _mm256_sub(x, SIXES);

    __m256fp lx = _mm256_log(x);
    __m256fp lx1 = _mm256_log(x1);
    __m256fp lx2 = _mm256_log(x2);
    __m256fp lx3 = _mm256_log(x3);
    __m256fp lx4 = _mm256_log(x4);
    __m256fp lx5 = _mm256_log(x5);
    __m256fp lx6 = _mm256_log(x6);

    // EXPLOITATIVE ASSOCIATION
    __m256fp xh_lx = _mm256_mul(xh, lx);
    __m256fp cz = _mm256_add(LGAM_CONST, z);
    __m256fp lx12 = _mm256_add(lx1, lx2);
    __m256fp lx34 = _mm256_add(lx3, lx4);
    __m256fp lx56 = _mm256_add(lx5, lx6);

    __m256fp r = _mm256_sub(xh_lx, x);
    __m256fp lx1234 = _mm256_add(lx12, lx34);

    __m256fp s = _mm256_add(r, cz);
    __m256fp lx1_6 = _mm256_add(lx1234, lx56);

    __m256fp t = _mm256_sub(s, lx1_6);

    stop_timer(rdtsc);
    return t;
}


/*
 * argmax
 *
 */

int argmax(fp_t* x, int n)
{
    int i;
    fp_t max = x[0];
    int argmax = 0;
    for (i = 1; i < n; i++)
    {
        if (x[i] > max)
        {
            max = x[i];
            argmax = i;
        }
    }
    return(argmax);
}
