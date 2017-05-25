#ifndef UTILS_H
#define UTILS_H

/* Precompiled header containing definitions of various util functions. */

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>

#include "fp.h"
#include "rdtsc-helper.h"

#ifdef FORCE_INLINE
    // Tell the compiler to inline this at all costs.
    #ifndef __INTEL_COMPILER
        #define INLINE inline __attribute__((always_inline))
    #else
        // There is a pragrma in icc, however I can't get it to work right now.
        #define INLINE inline
    #endif // __INTEL_COMPILER
#else
    // Only tell the compiler we'd like this to be inlined.
    // Note: icc seems to be more eager to inline than gcc
    #define INLINE inline
#endif // FORCE_INLINE


// This is not really a performance-critical function.
int argmax(fp_t* x, int n);

#if defined(NO_MKL) || !defined(__INTEL_COMPILER)
    INLINE
    void vdLGamma(int stride, const fp_t* input, fp_t* output){
        for(int i=0;i<stride;i++)
        {
            output[i] = lgamma(input[i]);
        }
    }
#else
    #include "mkl.h"
#endif //defined(NO_MKL) || !defined(__INTEL_COMPILER)


#ifndef __INTEL_COMPILER
    /* These are some replacements for the icc-specific intrinsics. The
    performance is of course gonna be much worse, but they're here so that we
    can at least compile on gcc. */

    INLINE
    __m256fp _mm256_log(__m256fp x) {
        fp_t* vals = (fp_t*) &x;

        // This is pretty terrible but it's basically the only thing we can do.
        for (int i = 0 ; i < STRIDE ; i++)
            vals[i] = (fp_t) log(vals[i]);

        return x;
    }

    INLINE
    __m256fp _mm256_exp(__m256fp x) {
        fp_t* vals = (fp_t*) &x;

        // This is pretty terrible but it's basically the only thing we can do.
        for (int i = 0 ; i < STRIDE ; i++)
            vals[i] = (fp_t) exp(vals[i]);

        return x;
    }

    INLINE
    __m256fp _mm256_log1p(__m256fp x) {
        fp_t* vals = (fp_t*) &x;

        // This is pretty terrible but it's basically the only thing we can do.
        for (int i = 0 ; i < STRIDE ; i++)
            vals[i] = (fp_t) log1p(vals[i]);

        return x;
    }
#endif // __INTEL_COMPILER


// ======== Non-vector argument math functions ========
// ====================================================

/* given log(a) and log(b), return log(a + b) */

INLINE
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
 */
INLINE
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


/* taylor approximation of first derivative of the log gamma function. */
INLINE
fp_t digamma(fp_t x)
{
    // (??+, ?*, 8/, 1 log) 126 cycles
    timer rdtsc = start_timer(DIGAMMA);

    fp_t p, p_sq, z, t0, t1, t2, t3, t4, t5;
    z=x + 6;
    p=1 / (z*z);
    p_sq = p*p;

    const fp_t a6 = + 0.2531135531135531,
               a5 = - 0.007575757575757576,
               a4 = + 0.004166666666667,
               a3 = - 0.003968253986254,
               a2 = + 0.008333333333333,
               a1 = - 0.08333333333333;

    t4 = a4 * p_sq + a3 * p;
    t3 = p_sq * t4;
    t2 = a2 * p_sq + a1 * p;
    t1 = t2 + t3;
    t0 = log(z) - 0.5/z - 1/(z-1) -1/(z-2) -1/(z-3) -1/(z-4) -1/(z-5) -1/(z-6);
    p = t1 + t0;

    stop_timer(rdtsc);
    return p;
}

INLINE
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


// ======== Vector-argument math functions ========
// ================================================

// INLINE
// __m256fp approx_log1p(__m256fp x)
// {
//     __m256fp a1 = _mm256_set1(0.99949556);
//     __m256fp a2 = _mm256_set1(-0.49190896);
//     __m256fp a3 = _mm256_set1(0.28947478);
//     __m256fp a4 = _mm256_set1(-0.13606275);
//     __m256fp a5 = _mm256_set1(0.03215845);
    
//     __m256fp x_squared = _mm256_mul(x, x);
//     __m256fp x_4 = _mm256_mul(x_squared, x_squared);

//     __m256fp t1 = _mm256_mul(a1, x);
//     __m256fp t2 = _mm256_mul(a2, x_squared);
//     __m256fp t3 = _mm256_mul(a3, _mm256_mul(x_squared, x));
//     __m256fp t4 = _mm256_mul(a4, x_4);
//     __m256fp t5 = _mm256_mul(a5, _mm256_mul(x_4, x));
            
//     t1 = _mm256_add(t1, t2);
//     t1 = _mm256_add(t1, t3);
//     t1 = _mm256_add(t1, t4);
//     t1 = _mm256_add(t1, t5);

//     return t1;
// }

// INLINE
// __m256fp looped_log1p(__m256fp x)
// {
//     fp_t input[4];
//     _mm256_storeu(input, x);
//     for(int i=0;i<4;i++)
//     {
//         input[i] = log1p(input[i]);
//     }

//     __m256fp output = _mm256_loadu(input);
//     return output;
// }


/* given log(a) and log(b), return log(a + b) */
INLINE
__m256fp log_sum_vec(__m256fp log_a, __m256fp log_b)
{
   // v = log_b+log(1 + exp(log_a-log_b));
  timer rdtsc = start_timer(LOG_SUM);

  // used for straightforward impl. __m256fp ones = _mm256_set1(1);
  __m256fp res = _mm256_sub(log_a, log_b);
  res = _mm256_exp(res);
  //  used for straightforward impl. res = _mm256_add(ones, res);
  res = _mm256_log1p(res);
  //  used for straightforward impl. res = _mm256_log(res);
  res = _mm256_add(log_b, res);

  stop_timer(rdtsc);
  return res;
}

INLINE 
__m256fp log_sum_vec_masked(__m256fp log_a, __m256fp log_b, __m256i mask)
{
    __m256fp v_log_sum = log_sum_vec(log_a, log_b);
    __m256fp v_mask = _mm256_castsi256(mask);
    // We need to chose if we had a log to add or the entry was masked and then 
    // we just take the log_a which was computed already, because we cannot do
    // log(x + 0).
    __m256fp res = _mm256_blendv(log_a, v_log_sum, v_mask);

    return res;
}

INLINE
__m256fp digamma_vec(__m256fp x)
{   // 190 cycles :(
    timer rdtsc = start_timer(DIGAMMA);

    __m256fp SIXES = _mm256_set1(6);

    // z = x + 6;
    __m256fp z  = _mm256_add(x, SIXES);
    // p = 1 / (z*z)
    __m256fp zsq = _mm256_mul(z, z);
    __m256fp p   = _mm256_rcp(zsq);
    // p_sq = p*p;
    __m256fp p_sq = _mm256_mul(p, p);


    // const fp_t a4 = ...
    __m256fp a4 = _mm256_set1((fp_t) + 0.004166666666667);
    __m256fp a3 = _mm256_set1((fp_t) - 0.003968253986254);
    __m256fp a2 = _mm256_set1((fp_t) + 0.008333333333333);
    __m256fp a1 = _mm256_set1((fp_t) - 0.08333333333333);

    //t4 = a4 * p_sq + a3 * p;
    __m256fp a4psq = _mm256_mul(a4, p_sq);
    __m256fp a3p = _mm256_mul(a3, p);
    __m256fp t4 = _mm256_add(a4psq, a3p);
    // t3 = p_sq * t4;
    __m256fp t3 = _mm256_mul(p_sq, t4);
    // t2 = a2 * p_sq + a1 * p;
    __m256fp a2psq = _mm256_mul(a2, p_sq);
    __m256fp a1p = _mm256_mul(a1, p);
    __m256fp t2 = _mm256_add(a2psq, a1p);
    // t1 = t2 + t3;
    __m256fp t1 = _mm256_add(t2, t3);

    // t0 = log(z) - 0.5/z - 1/(z-1) -1/(z-2) -1/(z-3) -1/(z-4) -1/(z-5) -1/(z-6);
    __m256fp HALVES = _mm256_set1(0.5);
    __m256fp ONES = _mm256_set1(1);
    __m256fp TWOS = _mm256_set1(2);
    __m256fp THREES = _mm256_set1(3);
    __m256fp FOURS = _mm256_set1(4);
    __m256fp FIVES = _mm256_set1(5);

    __m256fp logz = _mm256_log(z);
    __m256fp hoz = _rcp_const(HALVES, z);

    __m256fp zm1 = _mm256_sub(z, ONES);
    __m256fp zm1r = _mm256_rcp(zm1);
    __m256fp zm2 = _mm256_sub(z, TWOS);
    __m256fp zm2r = _mm256_rcp(zm2);
    __m256fp zm12r = _mm256_add(zm1r, zm2r);
    __m256fp logz_m_hoz = _mm256_sub(logz, hoz);
    __m256fp a = _mm256_sub(logz_m_hoz, zm12r);
    __m256fp zm3 = _mm256_sub(z, THREES);
    __m256fp zm3r = _mm256_rcp(zm3);
    __m256fp zm4 = _mm256_sub(z, FOURS);
    __m256fp zm4r = _mm256_rcp(zm4);
    __m256fp zm34r = _mm256_add(zm3r, zm4r);
    __m256fp zm5 = _mm256_sub(z, FIVES);
    __m256fp zm5r = _mm256_rcp(zm5);
    __m256fp zm6 = _mm256_sub(z, SIXES);
    __m256fp zm6r = _mm256_rcp(zm6);
    __m256fp zm56r = _mm256_add(zm5r, zm6r);
    __m256fp b = _mm256_add(zm34r, zm56r);

    // Maximally exploit associativity
    __m256fp t0 = _mm256_sub(a, b);

    // p = t1 + t0;
    __m256fp result = _mm256_add(t1, t0);

    stop_timer(rdtsc);
    return result;
}

/* Masked version of digamma. I'm not entirely convinced that this needs to be
 * actually defined. */
INLINE
__m256fp digamma_vec_mask(__m256fp x, __m256i mask) {
    __m256fp dig = digamma_vec(x);
    return _mm256_and(dig, _mm256_castsi256(mask));
}

INLINE
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


// ======== Vector-argument math functions ========
// ================================================


// hsum(x): return a vector where all elements are set to the sum of elements of x.
#ifdef FLOAT
    INLINE
    __m256 hsum(__m256 x) {
        // [A,B,C,D,E,F,G,H] -> [AB, CD, AB, CD, EF, GH, EF, GH]
        x = _mm256_hadd_ps(x, x);
        // -> [A..D * 4, E..H * 4]
        x = _mm256_hadd_ps(x, x);

        // -> [(A..D, E..H) * 4]
        __m256i perm = _mm256_set_epi32(4,0,4,0,4,0,4,0);
        x = _mm256_permutevar8x32_ps(x, perm);

        // -> [A..H * 8]
        return _mm256_hadd_ps(x, x);
    }
#else
    INLINE
    __m256d hsum(__m256d x) {
        // [A, B, C, D] -> [AB, AB, CD, CD]
        x = _mm256_hadd_pd(x, x);
        // -> [AB, CD, AB, CD]
        // Immediate:
        // 11 01 10 00 = 216
        // ^  ^  ^  ^-- 1st dst = 1st src
        // |  |  +-- 2nd dst = 3rd src
        // |  +-- 3rd dst = 2nd dst
        // +-- 4th dst = 4th src
        x = _mm256_permute4x64_pd(x, 216);

        // -> [A..D * 4]
        return _mm256_hadd_pd(x, x);
    }
#endif // DOUBLE

// First element of this vector
#define first(x) *((fp_t*) &(x))

#endif // UTILS_H
