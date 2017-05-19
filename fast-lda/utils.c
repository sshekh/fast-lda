#include "utils.h"

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
