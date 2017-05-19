#include "fp.h"
#include <math.h>
#include <stdio.h>

#ifdef DOUBLE
    void printv(const char* s, __m256d x) {
        double* a = (double*) &x;
        printf("%s [%f %f %f %f]\n", s, a[0], a[1], a[2], a[3]);
    }
#else
    void printv(const char* s, __m256 x) {
        float* a = (float*) &x;
        printf("%s [%f %f %f %f %f %f %f %f]\n", s, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
    }
#endif
