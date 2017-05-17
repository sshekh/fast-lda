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