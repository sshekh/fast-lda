#include "fp.h"
#include <math.h>

#ifndef __INTEL_COMPILER
    /* These are some replacements for the icc-specific intrinsics. The
    performance is of course gonna be much worse, but they're here so that we
    can at least compile on gcc. */


    __m256fp _mm256_log(__m256fp x) {
        // This is pretty terrible but it's basically the only thing we can do.
        fp_t vals[STRIDE];
        _mm256_storeu(vals, x);

        for (int i = 0 ; i < STRIDE ; i++)
            vals[i] = (fp_t) log(vals[i]);

        return _mm256_loadu(vals);
    }

#endif


// hsum(x): return a vector where all elements are set to the sum of elements of x.
#ifdef DOUBLE
    inline __m256d hsum(__m256d x) {
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
#else
    inline __m256 hsum(__m256 x) {
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
#endif