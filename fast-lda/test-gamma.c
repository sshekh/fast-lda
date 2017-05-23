#define DOUBLE

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


void test_dg_batch(double* vals) {

    double ref[4];
    double res[4];
    for (int i = 0 ; i < 4 ; i ++)
        ref[i] = digamma(vals[i]);

    __m256fp vecvals = _mm256_loadu_pd(vals);
    __m256fp vecres = digamma_vec(vecvals);
    _mm256_storeu_pd(res, vecres);

    printf("digamma\n");
    printf("actual,computed\n");
    for (int i = 0 ; i < 4 ; i++) {
        printf("%.10lf %.10lf\n", ref[i], res[i]);
    }
    //printf("t2vec %lf %lf %lf %lf\n", ((double*)&t2)[0], ((double*)&t2)[1], ((double*)&t2)[2], ((double*)&t2)[3] );
    //printf("t2 = %lf\n", t2);
}

/*
void test_lg_batch(float* vals) {

    float ref_h[8];
    float ref_u[8];
    float res[8];
    for (int i = 0 ; i < 8 ; i ++) {
        ref_h[i] = lgamma(vals[i]);
        ref_u[i] = log_gamma(vals[i]);
    }

    __m256 vecvals = _mm256_loadu_ps(vals);
    __m256 vecres = log_gamma_vec(vecvals);
    _mm256_storeu_ps(res, vecres);

    printf("log_gamma\n");
    printf("lib,lda,computed\n");
    for (int i = 0 ; i < 8 ; i++) {
        printf("%f %f %f\n", ref_h[i], ref_u[i], res[i]);
    }
}

#ifdef DOUBLE
    void test_hsum() {
        double vals[4];
        for (int i = 0 ; i < 4 ; i++)
            vals[i] = rand() / (double) RAND_MAX * 3.0;

        __m256d vvals = _mm256_loadu_pd(vals);
        __m256d vres = hsum(vvals);

        double res[4];
        _mm256_storeu_pd(res, vres);

        printf("[%f %f %f %f] -> [%f %f %f %f]\n", vals[0], vals[1], vals[2], vals[3],
            res[0], res[1], res[2], res[3]);
    }
#else
    void test_hsum() {
        float vals[8];
        for (int i = 0 ; i < 8 ; i++)
            vals[i] = rand() / (float) RAND_MAX * 3.f;

        __m256 vvals = _mm256_loadu_ps(vals);
        __m256 vres = hsum(vvals);

        float res[8];
        _mm256_storeu_ps(res, vres);

        printf("[%f %f %f %f %f %f %f %f] -> [%f %f %f %f %f %f %f %f]\n",
            vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7],
            res[0], res[1], res[2], res[3], res[4], res[5], res[6], res[7]);
    }
#endif
*/

int main() {
    const float DOM = 3.f;
    srand(time(NULL));
    
    double vals[4];
    for (int i = 0 ; i < 4 ; i++)
        //vals[i] = (i + 1) * (i + 1);
        vals[i] = rand() / (double) RAND_MAX * DOM;

    test_dg_batch(vals);
    printf("\n");
    // test_lg_batch(vals);

    // test_hsum();

    return 0;
}

