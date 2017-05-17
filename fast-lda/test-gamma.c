#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void test_batch(float* vals) {

    float ref[8];
    float res[8];
    for (int i = 0 ; i < 8 ; i ++)
        ref[i] = digamma(vals[i]);

    __m256 vecvals = _mm256_loadu_ps(vals);
    __m256 vecres = digamma_vec(vecvals);
    _mm256_storeu_ps(res, vecres);

    printf("actual,computed\n");
    for (int i = 0 ; i < 8 ; i++) {
        printf("%f %f\n", ref[i], res[i]);
    }
}

int main() {
    const float DOM = 3.f;
    srand(time(NULL));

    float vals[8];
    for (int i = 0 ; i < 8 ; i++)
        vals[i] = rand() / (float) RAND_MAX * DOM;

    test_batch(vals);

    return 0;
}
