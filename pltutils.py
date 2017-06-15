#! /usr/bin/python
import re
import os

"""
Globals
PURIFY <- return pure flops/cycles
D, V <- #doc length, #voc size
USE_DOUBLES <- using doubles, stride is 4
"""
PURIFY = True
D = None
V = None
USE_DOUBLES = True

""""
Helpers
"""
def memoize(f):
    cache = {}
    return lambda *args: cache[args] if args in cache else cache.update({args: f(*args)}) or cache[args]

def vec_strides(fn):
    if fn in ["DIGAMMA", "LOG_SUM", "LOG_GAMMA"]:
        ans = 4 if USE_DOUBLES else 8
    else:
        ans = 1
    return ans

def isnz(x):
    return abs(x) > 1e-8

iters = {"EM_CONVERGE" : 1, "INFERENCE_CONVERGE" : 1., "ALPHA_CONVERGE" : 1.}  # conv counts for var iterations

"""
Every list returned is in this order
"""
fns = ["RUN_EM", "DOC_E_STEP", "LDA_INFERENCE", "LIKELIHOOD", "MLE", "LOG_SUM", "DIGAMMA", "OPT_ALPHA", "TRIGAMMA", "LOG_GAMMA"]

"""
Setting corpus stats like D, K
MUST BE CALLED BEFORE USING ANYTHING ELSE!!
"""
def set_corpus_stats(location):
    global D
    global V
    global USE_DOUBLES
    with open(location + '/info.txt') as f:
        found = False
        ln = 'This is the line'
        while ln != '':
            ln = f.readline()
            if ln.startswith('use_long'):
                found = True
                # A little bird told us that these were the proper values
                if ln.endswith('True\n'): # Long corpus
                    D = 51103
                    V = 196106
                else: # Regular corpus
                    D = 135
                    V = 10473
                break
            if ln.startswith('use_doubles'):
                if ln.endswith('False'):
                    USE_DOUBLES = False

        if not found:
            print('Warning: use_long setting not found, assuming ap corpus...')
            D = 135
            V = 10473

class Cost:
    def __init__(self, adds=0, muls=0, divs=0, logs=0, exps=0):
            self.adds = adds
            self.muls = muls
            self.divs = divs
            self.logs = logs
            self.exps = exps

    def toTup(self):
        return (self.adds, self.muls, self.divs, self.logs, self.exps)

    def __str__(self):
        return str(self.toTup())

    def __add__(self, other):
        return Cost(self.adds + other.adds, \
                    self.muls + other.muls, \
                    self.divs + other.divs, \
                    self.logs + other.logs, \
                    self.exps + other.exps)

    def __mul__(self, other):
        if isinstance(other, int) or isinstance(other, float):
            return Cost(self.adds * other, \
                        self.muls * other, \
                        self.divs * other, \
                        self.logs * other, \
                        self.exps * other)
        else:
            return NotImplemented     # multiplication not defined

    def __rmul__(self, other):
        return self.__mul__(other)

    def __radd__(self, other):
        return self.__add__(other)

    def full(self):
        # <BG> weighting according to file notes/benchmark_exp_log.txt
        # we use the benchmarked values for math.h macros and suppose that they run in latency mode
        # That leads us to lower bounds for FLOPS for both log and exp.
        # Skylake Latencies: mul, add: 4, div:13
        return  self.adds + \
                self.muls + \
                self.divs + \
                self.logs + \
                self.exps       # proposed reweighting: log: 3, exp: 14, (div: 3)

ZERO = Cost(0, 0, 0, 0, 0)

"""
Calculating flops
"""
#FIXME digamma2??
def digamma(N, K):
    return Cost(18, 5, 8, 1, 0)

def log_sum(N, K):
    return Cost(4, 0, 0, 1, 1)

#FIXME: Not the correct flops as the written function is not used, we use lgamma from math.h and vldGamma from mkl
def log_gamma(N, K):
    # This was calculated as a lower bound based on the measured latency, but it's apparently still way too low
    # Throughput mode: would be 72 cycles --> approx 18 adds
    # return Cost(7, 0, 0, 0, 0)
    return Cost(20, 5, 2, 7, 0)

def trigamma(N, K):
    return Cost(7, 7, 2, 0, 0) + 6 * Cost(2, 1, 1, 0, 0)

def random_initialize_ss(N, K):
    return K * N * Cost(adds=3, divs=1)

def opt_alpha(N, K):
    return iters["ALPHA_CONVERGE"] * ( \
            Cost(adds=2, muls=1, divs=1, exps=1) + Cost(adds=3, muls=4) + 2 * log_gamma(N, K) + \
            Cost(adds=2, muls=4) + 2 * digamma(N, K) + Cost(adds=1, muls=5) + 2 * trigamma(N, K)) + \
            Cost(exps=1) # poor lonely exp

def mle(N, K):
    return V * K * Cost(adds=2, logs=2) + opt_alpha(N, K)

def likelihood(N, K):
    return K * digamma(N, K) + Cost(adds=K) + digamma(N, K) + Cost(adds=2, muls=2) + 3 * log_gamma(N, K) + \
            K * Cost(adds=7, muls=2) + K * D * 6 * Cost(adds=4, muls=2, logs=1)

def lda_inference(N, K):
    return K * Cost(adds=1, divs=1) + K * digamma(N, K) + K * D * Cost(divs=1) + \
        iters["INFERENCE_CONVERGE"] * (D * K * (digamma(N, K) + Cost(adds=4, muls=1, exps=1) + log_sum(N, K)) + \
            likelihood(N, K) + Cost(adds=1, divs=1))

def doc_e_step(N, K):
    return lda_inference(N, K) + K * Cost(adds=2) + Cost(adds=1, muls=1) + (K + 1) * digamma(N, K) + K * D * Cost(adds=2, muls=2)

def run_em(N, K):
    return random_initialize_ss(N, K) + mle(N, K) + iters["EM_CONVERGE"] * (N * doc_e_step(N, K) + Cost(adds=N) + \
            mle(N, K) + Cost(adds=1, muls=1, divs=1))

flops = { "RUN_EM" : run_em, "LDA_INFERENCE" : lda_inference, "DIGAMMA" : digamma, "LOG_SUM" : log_sum,
        "LOG_GAMMA" : log_gamma, "TRIGAMMA" : trigamma, "DOC_E_STEP" : doc_e_step, "LIKELIHOOD" : likelihood, "MLE" : mle, "OPT_ALPHA" : opt_alpha}

"""
Calculating pure flops
"""
def pdigamma(N, K):
    return digamma(N, K)

def plog_sum(N, K):
    return log_sum(N, K)

def plog_gamma(N, K):
    return log_gamma(N, K)

def ptrigamma(N, K):
    return trigamma(N, K)

def prandom_initialize_ss(N, K):
    return random_initialize_ss(N, K)

def popt_alpha(N, K):
    return iters["ALPHA_CONVERGE"] * ( \
            Cost(adds=2, muls=1, divs=1, exps=1) + Cost(adds=3, muls=4) + 2 * ZERO + \
            Cost(adds=2, muls=4) + 2 * ZERO + Cost(adds=1, muls=5) + 2 * ZERO) + \
            Cost(exps=1) # poor lonely exp

def pmle(N, K):
    return V * K * Cost(adds=2, logs=2) + ZERO

def plikelihood(N, K):
    return K * ZERO + Cost(adds=K) + ZERO + Cost(adds=2, muls=2) + 3 * ZERO + \
            K * Cost(adds=7, muls=2) + K * D * 6 * Cost(adds=4, muls=2, logs=1)

def plda_inference(N, K):
    return K * Cost(adds=1, divs=1) + K * ZERO + K * D * Cost(divs=1) + \
        iters["INFERENCE_CONVERGE"] * (D * K * (ZERO + Cost(adds=4, muls=1, exps=1) + ZERO) + \
            ZERO + Cost(adds=1, divs=1))

def pdoc_e_step(N, K):
    return ZERO + K * Cost(adds=2) + Cost(adds=1, muls=1) + ZERO + K * D * Cost(adds=2, muls=2)

def prun_em(N, K):
    return random_initialize_ss(N, K) + ZERO + iters["EM_CONVERGE"] * (N * ZERO + Cost(adds=N) + \
            ZERO + Cost(adds=1, muls=1, divs=1))

pure_flops = { "RUN_EM" : prun_em, "LDA_INFERENCE" : plda_inference, "DIGAMMA" : pdigamma, "LOG_SUM" : plog_sum,
        "LOG_GAMMA" : plog_gamma, "TRIGAMMA" : ptrigamma, "DOC_E_STEP" : pdoc_e_step, "LIKELIHOOD" : plikelihood, "MLE" : pmle, "OPT_ALPHA" : popt_alpha}

"""
Calculating impure cycles
"""
def idigamma(N, K):
    return 0

def ilog_sum(N, K):
    return 0

def ilog_gamma(N, K):
    return 0

def itrigamma(N, K):
    return 0 + 6 * 0

def irandom_initialize_ss(N, K):
    return K * N * 0

def iopt_alpha(N, K):
    return iters["ALPHA_CONVERGE"] * ( \
            0 + 0 + 2 * avg_cycles["LOG_GAMMA"] + \
            0 + 2 * avg_cycles["DIGAMMA"] + 0 + 2 * avg_cycles["TRIGAMMA"]) + \
            0 # poor lonely exp

def imle(N, K):
    return V * K * 0 + avg_cycles["OPT_ALPHA"]

def ilikelihood(N, K):
    return K * avg_cycles["DIGAMMA"] + 0 + avg_cycles["DIGAMMA"] + 0 + 3 * avg_cycles["LOG_GAMMA"] + \
            K * 0 + K * D * 6 * 0

def ilda_inference(N, K):
    return K * 0 + K * avg_cycles["DIGAMMA"] + K * D * 0 + \
        iters["INFERENCE_CONVERGE"] * (D * K * (avg_cycles["DIGAMMA"] + 0 + avg_cycles["LOG_SUM"]) + \
            avg_cycles["LIKELIHOOD"] + 0)

def idoc_e_step(N, K):
    return avg_cycles["LDA_INFERENCE"] + K * 0 + 0 + (K + 1) * avg_cycles["DIGAMMA"] + K * D * 0

def irun_em(N, K):
    return 0 + avg_cycles["MLE"] + iters["EM_CONVERGE"] * (N * avg_cycles["DOC_E_STEP"] + 0 + \
            avg_cycles["MLE"] + 0)

impurity = { "RUN_EM" : irun_em, "LDA_INFERENCE" : ilda_inference, "DIGAMMA" : idigamma, "LOG_SUM" : ilog_sum,
        "LOG_GAMMA" : ilog_gamma, "TRIGAMMA" : itrigamma, "DOC_E_STEP" : idoc_e_step, "LIKELIHOOD" : ilikelihood, "MLE" : imle, "OPT_ALPHA" : iopt_alpha}

tot_cycles = {"RUN_EM" : 0., "LDA_INFERENCE" : 0., "DIGAMMA" : 0., "LOG_SUM" : 0., "DOC_E_STEP" : 0., "LIKELIHOOD" : 0., "MLE" : 0., \
        "OPT_ALPHA" : 0., "TRIGAMMA" : 0., "LOG_GAMMA" : 0.}
pur_cycles = {"RUN_EM" : 0., "LDA_INFERENCE" : 0., "DIGAMMA" : 0., "LOG_SUM" : 0., "DOC_E_STEP" : 0., "LIKELIHOOD" : 0., "MLE" : 0., \
        "OPT_ALPHA" : 0., "TRIGAMMA" : 0., "LOG_GAMMA" : 0.}
avg_cycles = {"RUN_EM" : 0., "LDA_INFERENCE" : 0., "DIGAMMA" : 0., "LOG_SUM" : 0., "DOC_E_STEP" : 0., "LIKELIHOOD" : 0., "MLE" : 0., \
        "OPT_ALPHA" : 0., "TRIGAMMA" : 0., "LOG_GAMMA" : 0.}

"""
Read one timings file and calculate shit ton of stuff
"""
def read_one_output(fname, vec=False, debug=False):
    if debug:
        print('\n', fname, '\n')
    regex = re.compile(r'\d+')
    pname = os.path.basename(fname)
    K, N = map(int, re.findall(regex, pname))
    f = open(fname, "r")
    header = f.readline().split(',')
    header = [h.strip() for h in header]
    assert header[0] == 'Accumulator' \
        and header[1] == 'Total count' \
       and header[2] == 'Average count', "Output file not in proper format"
    lines = f.readlines()
    f.close()

    for fn in fns:
        tot_cycles[fn] = 0.
        pur_cycles[fn] = 0.
        avg_cycles[fn] = 0.
        pass
    for itr in iters: iters[itr] = 0.

    # Naming convention: All convergence counters should include the tag 'CONVERGE'
    for i,line in enumerate(lines):
        s = line.split(',')
        fn = s[0].strip()
        if 'CONVERGE' not in fn:
            if s[2].strip() is not '0':     # code inside function not called case
                tot_cnt = float(s[1])
                avg_cnt = float(s[2])
                tot_cycles[fn] = tot_cnt
                avg_cycles[fn] = avg_cnt
        else:
            iters[fn] = float(s[2])

    ret_tot_cycle_list = [0] * len(fns)
    ret_avg_cycle_list = [0] * len(fns)
    ret_flop_list = [0] * len(fns)
    ret_perf_list = [0] * len(fns)
    for i, fn in enumerate(fns):
        if PURIFY:
            pur_cycles[fn] = avg_cycles[fn] - impurity[fn](N, K)
            ret_avg_cycle_list[i] = pur_cycles[fn]
            ret_flop_list[i] = pure_flops[fn](N, K).full()
        else:
            ret_avg_cycle_list[i] = avg_cycles[fn]
            ret_flop_list[i] = flops[fn](N, K).full()
        if isnz(avg_cycles[fn]):
            ret_tot_cycle_list[i] = (tot_cycles[fn] / avg_cycles[fn]) * ret_avg_cycle_list[i]
            if vec:
                ret_perf_list[i] = (ret_flop_list[i] * vec_strides(fn)) / ret_avg_cycle_list[i]
            else:
                ret_perf_list[i] = ret_flop_list[i] / ret_avg_cycle_list[i]

    if debug:
        print(fns)
        print(ret_avg_cycle_list)
        print([str(flops[fn](N, K).full()) for fn in fns])
    return K, N, ret_flop_list, ret_tot_cycle_list, ret_avg_cycle_list, ret_perf_list



