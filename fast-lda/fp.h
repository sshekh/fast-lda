#ifndef VECTORIZE_H
#define VECTORIZE_H

#include <immintrin.h>

#ifdef DOUBLE
    #define fp_t double
    #define STRIDE 4

    #define STRIDE_SPLIT(n, q, m) {\
        *(q) = (n) - ((n) % 4);\
        switch ((n) % 4) {\
            case 0: *(m) = _mm256_setzero_si256(); break;\
            case 1: *(m) = _mm256_set_epi64x(0, 0, 0, 0xFFFFFFFFFFFFFFFF); break;\
            case 2: *(m) = _mm256_set_epi64x(0, 0, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF); break;\
            case 3: *(m) = _mm256_set_epi64x(0, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF); break;\
        }\
    }

    #define LEFTOVER(n) ((n) % 4)

    #define __m256fp            __m256d

    #define _mm256_add          _mm256_add_pd
    #define _mm256_addsub       _mm256_addsub_pd
    #define _mm256_and          _mm256_and_pd
    #define _mm256_andnot       _mm256_andnot_pd
    #define _mm256_blendv       _mm256_blendv_pd
    #define _mm256_broadcast    _mm256_broadcast_pd
    #define _mm256_broadcast_sd _mm256_broadcast_sd_pd
    #define _mm_broadcastsd     _mm_broadcastsd_pd
    #define _mm256_broadcastsd  _mm256_broadcastsd_pd
    #define _mm256_castsi256    _mm256_castsi256_pd
    #define _mm256_ceil         _mm256_ceil_pd
    #define _mm_cmp             _mm_cmp_pd
    #define _mm256_cmp          _mm256_cmp_pd
    #define _mm_cmp_sd          _mm_cmp_sd_pd
    #define _mm256_div          _mm256_div_pd
    #define _mm256_floor        _mm256_floor_pd
    #define _mm256_fmadd        _mm256_fmadd_pd
    #define _mm256_insertf128   _mm256_insertf128_pd
    #define _mm256_load         _mm256_load_pd
    #define _mm256_loadu        _mm256_loadu_pd
    #define _mm_maskload        _mm_maskload_pd
    #define _mm256_maskload     _mm256_maskload_pd
    #define _mm256_maskstore    _mm256_maskstore_pd
    #define _mm256_max          _mm256_max_pd
    #define _mm256_min          _mm256_min_pd
    #define _mm256_movemask     _mm256_movemask_pd
    #define _mm256_mul          _mm256_mul_pd
    #define _mm256_or           _mm256_or_pd
    #define _mm256_round        _mm256_round_pd
    #define _mm256_set1         _mm256_set1_pd
    #define _mm256_setzero      _mm256_setzero_pd
    #define _mm256_sqrt         _mm256_sqrt_pd
    #define _mm256_store        _mm256_store_pd
    #define _mm256_storeu       _mm256_storeu_pd
    #define _mm256_sub          _mm256_sub_pd
    #define _mm256_undefined    _mm256_undefined_pd
    #define _mm256_xor          _mm256_xor_pd

    #ifdef __INTEL_COMPILER
        #define _mm256_log          _mm256_log_pd
    #else
        // GCC doesn't have this intrinsic
        __m256d _mm256_log(__m256d x);
    #endif

    #define _mm256_rcp(a)       _mm256_div_pd(_mm256_set1_pd(1.0), a)

    // reciprocal * constant
    #define _rcp_const(c, a)    _mm256_div_pd(c, a)

#else
    #define fp_t float
    #define STRIDE 8

    #define STRIDE_SPLIT(n, q, m) {\
        *(q) = (n) - ((n) % 8);\
        switch ((n) % 8) {\
            case 0: *(m) = _mm256_setzero_si256(); break;\
            case 1: *(m) = _mm256_set_epi32(0,0,0,0,0,0,0,0xFFFFFFFF); break;\
            case 2: *(m) = _mm256_set_epi32(0,0,0,0,0,0, 0xFFFFFFFF, 0xFFFFFFFF); break;\
            case 3: *(m) = _mm256_set_epi32(0,0,0,0,0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF); break;\
            case 4: *(m) = _mm256_set_epi32(0,0,0,0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF); break;\
            case 5: *(m) = _mm256_set_epi32(0,0,0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF); break;\
            case 6: *(m) = _mm256_set_epi32(0,0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF); break;\
            case 7: *(m) = _mm256_set_epi32(0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF); break;\
        }\
    }

    #define LEFTOVER(n) ((n) % 8)

    #define __m256fp            __m256

    #define _mm256_add          _mm256_add_ps
    #define _mm256_addsub       _mm256_addsub_ps
    #define _mm256_and          _mm256_and_ps
    #define _mm256_andnot       _mm256_andnot_ps
    #define _mm256_blendv       _mm256_blendv_ps
    #define _mm256_broadcast    _mm256_broadcast_ps
    #define _mm256_broadcast_sd _mm256_broadcast_sd_ps
    #define _mm_broadcastsd     _mm_broadcastsd_ps
    #define _mm256_broadcastsd  _mm256_broadcastsd_ps
    #define _mm256_castsi256    _mm256_castsi256_ps
    #define _mm256_ceil         _mm256_ceil_ps
    #define _mm_cmp             _mm_cmp_ps
    #define _mm256_cmp          _mm256_cmp_ps
    #define _mm_cmp_sd          _mm_cmp_sd_ps
    #define _mm256_div          _mm256_div_ps
    #define _mm256_floor        _mm256_floor_ps
    #define _mm256_fmadd        _mm256_fmadd_ps
    #define _mm256_insertf128   _mm256_insertf128_ps
    #define _mm256_load         _mm256_load_ps
    #define _mm256_loadu        _mm256_loadu_ps
    #define _mm_maskload        _mm_maskload_ps
    #define _mm256_maskload     _mm256_maskload_ps
    #define _mm256_maskstore    _mm256_maskstore_ps
    #define _mm256_max          _mm256_max_ps
    #define _mm256_min          _mm256_min_ps
    #define _mm256_movemask     _mm256_movemask_ps
    #define _mm256_mul          _mm256_mul_ps
    #define _mm256_or           _mm256_or_ps
    #define _mm256_round        _mm256_round_ps
    #define _mm256_set1         _mm256_set1_ps
    #define _mm256_setzero      _mm256_setzero_ps
    #define _mm256_sqrt         _mm256_sqrt_ps
    #define _mm256_store        _mm256_store_ps
    #define _mm256_storeu       _mm256_storeu_ps
    #define _mm256_sub          _mm256_sub_ps
    #define _mm256_undefined    _mm256_undefined_ps
    #define _mm256_xor          _mm256_xor_ps


    #define _mm256_rcp          _mm256_rcp_ps
    // c * 1/a is faster than c / a when using the rcp instruction
    #define _rcp_const(c, a)    _mm256_mul_ps(c, _mm256_rcp_ps(a))

    #ifdef __INTEL_COMPILER
        #define _mm256_log          _mm256_log_ps
    #else
        // GCC doesn't have this intrinsic
        __m256 _mm256_log(__m256 x);
    #endif

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


inline fp_t first(__m256fp x) {
    fp_t a[STRIDE];
    _mm256_storeu(a, x);
    return a[0];
}


#endif