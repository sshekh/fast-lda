#include "utils.h"
#include "../fast-lda/rdtsc-helper.h"


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
