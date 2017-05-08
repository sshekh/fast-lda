#! /usr/bin/python
import numpy as np
import sys
import matplotlib.pyplot as plt
from os import listdir
from os.path import join
import re

EPS = 1e-12     # Average count is float and integer count is a text in case of 0 calls

D = 135
# A little bird told us that this is the proper value
V = 10473

def memoize(f):
    cache = {}
    return lambda *args: cache[args] if args in cache else cache.update({args: f(*args)}) or cache[args]

class Cost:
    def __init__(self, simple, heavy):
        self.simple = simple
        self.heavy = heavy

    def __str__(self):
        return "(" + str(self.simple) + ", " + str(self.heavy) + ")"

    def __add__(self, other):
        return Cost(self.simple + other.simple,  \
                self.heavy + other.heavy)

    def __mul__(self, other):
        if type(other) == type(4.):
            return Cost(self.simple * scalar, \
                self.heavy * scalar)
        else: 
            return Cost(0, 0)

    def full(self):
        return self.simple + 10 * self.heavy

iters = {"EM_CONVERGE" : 1,
        "INFERENCE_CONVERGE" : 1. }   # EM_CONVERGE and INFERENCE_CONVERGE are # iteration counts for convergence

@memoize
def digamma(N, K):
    return Cost(23, 9)

@memoize
def log_sum(N, K):
    return Cost(4, 2)

@memoize
def log_gamma(N, K):
    return Cost(25, 9)

@memoize
def random_initialize_ss(N, K):
    return Cost(2*K*N, 2*K*N)

@memoize
def lda_mle(N, K):
    return Cost(K*V, 2*K*V) 

@memoize
def likelihood(N, K):
    return Cost((K + 1) * digamma(N, K).simple + 3 * log_gamma(N, K).simple + 6*K*D + 10*K + 4, \
            (K + 1) * digamma(N, K).heavy + 3 * log_gamma(N, K).heavy)

@memoize
def lda_inference(N, K):
    return Cost(K * digamma(N, K).simple + K + iters["INFERENCE_CONVERGE"] * (D * K * (digamma(N, K).simple + log_sum(N, K).simple + 5)) \
                    + likelihood(N, K).simple + 2,  \
            K + K * digamma(N, K).heavy + K*D + iters["INFERENCE_CONVERGE"] * (D * K * (digamma(N, K).heavy + log_sum(N, K).heavy + 1)))
    #return Cost (24*K + iters["INFERENCE_CONVERGE"]*(38*K*D + 33*K + 104), \
    #        K*D + 10*K + iters["INFERENCE_CONVERGE"]*(14*K*D + 9*K + 36))

@memoize
def doc_e_step(N, K):
    return Cost(lda_inference(N, K).simple + digamma(N, K).simple + 4*K*N + 2*K + 3, \
            lda_inference(N, K).heavy + digamma(N, K).heavy)
    #return Cost(4*K*N + 26*K + 26 + lda_inf)

@memoize
def run_em(N, K):
    return Cost(random_initialize_ss(N, K).simple + lda_mle(N, K).simple +  \
        + iters["EM_CONVERGE"] * (N * doc_e_step(N, K).simple + N + lda_mle(N, K).simple + 2 + \
                                     N * lda_inference(N, K).simple), \
                random_initialize_ss(N, K).heavy + lda_mle(N, K).heavy +  \
        + iters["EM_CONVERGE"] * (N * doc_e_step(N, K).heavy + lda_mle(N, K).heavy + 1 + \
                                     N * lda_inference(N, K).heavy))

flops = { "RUN_EM" : run_em, "LDA_INFERENCE" : lda_inference, "DIGAMMA" : digamma, "LOG_SUM" : log_sum,
        "LOG_GAMMA" : log_gamma, "DOC_E_STEP" : doc_e_step, "LIKELIHOOD" : likelihood }

def plot(plt, data, color):
    for fn, vals in data.items():
        if len(vals['x']) is not 0:
            x, y = (zip(*sorted(zip(vals['x'], vals['y']))))
            plt.plot(x, y, '^-', color=color, linewidth=2)
            plt.text(x[0], y[0] + 0.002 , fn)        # label 

def read_one_output(f, N, K, dict):
    header = f.readline().split(',')
    header = [h.strip() for h in header]
    assert header[0] == 'Accumulator' \
        and header[1] == 'Total count' \
       and header[2] == 'Average count', "Output file not in proper format"
    lines = f.readlines()
    assert len(lines) == 9, "Timings file has incompatible # lines"

    for i in range(7, 9):
        s = lines[i].split(',')
        fn = s[0].strip()
        iters[fn] = float(s[2])

    for i in range(7):
        s = lines[i].split(',')
        fn = s[0].strip()
        if not fn in dict: dict[fn] = { 'x' : [], 'y' : []}
        avg_cnt = float(s[2])
        if abs(avg_cnt) > EPS:
            dict[fn]['x'].append(N)
            dict[fn]['y'].append(flops[fn](N, K).full() / avg_cnt)
        pass
    pass

def benchmark(dirpath):
    data_1 = {}       # "RUN_EM" : { x : [10, 15, 20], y : [3.2, 4.5, 6.7] }
    data_2 = {}       # "RUN_EM" : { x : [10, 15, 20], y : [3.2, 4.5, 6.7] }

    _, axes = plt.subplots()

    regex = re.compile(r'\d+')
    for filename in listdir(dirpath):
        if filename.startswith("fast") or filename.startswith("slow"):
            f = open(join(dirpath, filename), "r")
            N, K = map(int, re.findall(regex, filename))
            if filename[0] == 'f':
                read_one_output(f, N, K, data_1)
            else:
                read_one_output(f, N, K, data_2)

    # plot roof
    #plt.axhline(y=4, linestyle='--', color='black', label='Compute roof')
    plt.ylabel('performance (flops/cycle)')
    plt.xlabel('number of documents')

    axes.yaxis.grid(color='white', linestyle='solid')
    axes.set_facecolor((211.0/255,211.0/255,211.0/255))
    
    plot(plt, data_1, color='blue')
    plot(plt, data_2, color='red')
    plt.show()

if __name__ == '__main__':
    benchmark(sys.argv[1])
    

