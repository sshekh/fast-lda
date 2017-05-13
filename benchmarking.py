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
    def __init__(self, adds=0, muls=0, divs=0, logs=0, exps=0):
        if isinstance(adds, tuple):
            self.adds = adds[0]
            self.muls = adds[1]
            self.divs = adds[2]
            self.logs = adds[3]
            self.exps = adds[4]
        else:
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
        if isinstance(other, tuple):
            other = Cost(other)
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
        return  self.adds + \
                self.muls + \
                self.divs + \
                self.logs + \
                self.exps 

iters = {"EM_CONVERGE" : 1, "INFERENCE_CONVERGE" : 1., "ALPHA_CONVERGE" : 1.}  # conv counts for var iterations 

@memoize
def digamma(N, K):
    return Cost(18, 5, 8, 1, 0) 

@memoize
def log_sum(N, K):
    return Cost(4, 0, 0, 1, 1)

@memoize
def log_gamma(N, K):
    return Cost(20, 5, 2, 7, 0)  

@memoize
def trigamma(N, K):
    return Cost(7, 7, 2, 0, 0) + 6 * Cost(2, 1, 1, 0, 0) 

@memoize
def random_initialize_ss(N, K):
    return K * N * Cost(adds=3, divs=1)

@memoize
def opt_alpha(N, K):
    return iters["ALPHA_CONVERGE"] * ( \
            Cost(adds=2, muls=1, divs=1, exps=1) + Cost(adds=3, muls=4) + 2 * log_gamma(N, K) + \
            Cost(adds=2, muls=4) + 2 * digamma(N, K) + Cost(adds=1, muls=5) + 2 * trigamma(N, K)) + \
            Cost(exps=1) # poor lonely exp

@memoize
def mle(N, K):
    return N * K * Cost(adds=2, logs=2) + opt_alpha(N, K)

@memoize
def likelihood(N, K):
    return K * digamma(N, K) + Cost(adds=K) + digamma(N, K) + Cost(adds=2, muls=2) + 3 * log_gamma(N, K) + \
            K * Cost(adds=7, muls=2) + K * D * 6 * Cost(adds=4, muls=2, logs=1)
    #return Cost((K + 1) * digamma(N, K).simple + 3 * log_gamma(N, K).simple + 6*K*D + 10*K + 4, \
    #        (K + 1) * digamma(N, K).heavy + 3 * log_gamma(N, K).heavy)

@memoize
def lda_inference(N, K):
    return K * Cost(adds=1, divs=1) + K * digamma(N, K) + K * D * Cost(divs=1) + \
        iters["INFERENCE_CONVERGE"] * (D * K * (digamma(N, K) + Cost(adds=4, muls=1, exps=1) + log_sum(N, K)) + \
            likelihood(N, K) + Cost(adds=2))
    #return Cost(K * digamma(N, K).simple + K + iters["INFERENCE_CONVERGE"] * (D * K * (digamma(N, K).simple + log_sum(N, K).simple + 5)) \
    #                + likelihood(N, K).simple + 2,  \
    #        K + K * digamma(N, K).heavy + K*D + iters["INFERENCE_CONVERGE"] * (D * K * (digamma(N, K).heavy + log_sum(N, K).heavy + 1)))

@memoize
def doc_e_step(N, K):
    return lda_inference(N, K) + K * Cost(adds=2) + Cost(adds=1, muls=1) + digamma(N, K) + K * N * Cost(adds=2, muls=2)
    #return Cost(lda_inference(N, K).simple + digamma(N, K).simple + 4*K*N + 2*K + 3, \
    #        lda_inference(N, K).heavy + digamma(N, K).heavy)

@memoize
def run_em(N, K):
    return random_initialize_ss(N, K) + mle(N, K) + iters["EM_CONVERGE"] * (N * doc_e_step(N, K) + Cost(adds=N) + \
            mle(N, K) + Cost(adds=1, muls=1, divs=1) + N * lda_inference(N, K))
    #return Cost(random_initialize_ss(N, K).simple + mle(N, K).simple +  \
    #    + iters["EM_CONVERGE"] * (N * doc_e_step(N, K).simple + N + mle(N, K).simple + 2 + \
    #                                 N * lda_inference(N, K).simple), \
    #            random_initialize_ss(N, K).heavy + mle(N, K).heavy +  \
    #    + iters["EM_CONVERGE"] * (N * doc_e_step(N, K).heavy + mle(N, K).heavy + 1 + \
    #                                 N * lda_inference(N, K).heavy))

flops = { "RUN_EM" : run_em, "LDA_INFERENCE" : lda_inference, "DIGAMMA" : digamma, "LOG_SUM" : log_sum,
        "LOG_GAMMA" : log_gamma, "TRIGAMMA" : trigamma, "DOC_E_STEP" : doc_e_step, "LIKELIHOOD" : likelihood, "MLE" : mle, "OPT_ALPHA" : opt_alpha}

colors = { "RUN_EM" : "green", "LDA_INFERENCE" : "blue", "DIGAMMA" : "black", "LOG_SUM" : "purple",
        "LOG_GAMMA" : "purple", "TRIGAMMA" : "cyan", "DOC_E_STEP" : "orange", "LIKELIHOOD" : "red", "MLE" : "cyan", "OPT_ALPHA" : "cyan"}       

label_offsets = { "RUN_EM" : 0.008, "LDA_INFERENCE" : 0.008, "DIGAMMA" : 0.02, "LOG_SUM" : -0.015,
        "LOG_GAMMA" : 0.008, "TRIGAMMA" : 0.01, "DOC_E_STEP" : -0.01, "LIKELIHOOD" : -0.015, "MLE" : 0.01, "OPT_ALPHA" : 0.01 }                 



def read_one_output(f, N, K, dict):
    header = f.readline().split(',')
    header = [h.strip() for h in header]
    assert header[0] == 'Accumulator' \
        and header[1] == 'Total count' \
       and header[2] == 'Average count', "Output file not in proper format"
    lines = f.readlines()


    # Naming convention: All convergence counters should include the tag 'CONVERGE'
    for i in range(len(lines)):
        s = lines[i].split(',')
        fn = s[0].strip()
        if 'CONVERGE' in fn:
            iters[fn] = float(s[2])

    for i in range(len(lines)):
        s = lines[i].split(',')
        fn = s[0].strip()
        if 'CONVERGE' not in fn:
            if not fn in dict: dict[fn] = { 'x' : [], 'y' : []}
            avg_cnt = float(s[2])
            if abs(avg_cnt) > EPS:
                dict[fn]['x'].append(N)
                dict[fn]['y'].append(flops[fn](N, K).full() / avg_cnt)
            pass
        pass
    pass

def plot_line(plt, data, color):
    for fn, vals in data.items():
        if len(vals['x']) is not 0:
            x, y = (zip(*sorted(zip(vals['x'], vals['y']))))
            plt.plot(x, y, '^-', color=colors[fn], linewidth=1)
            plt.text(x[0], y[0] + label_offsets[fn] , fn, color=colors[fn], size=9)

def set_up_perf_plot(axes):
    #Per plot settings
    axes.set_title('Performance on Haswell',  y=1.08, loc = "center")
    axes.set_xlabel('$n$ documents')
    axes.set_ylabel('Performance [flops/cycles]',rotation="0")
    axes.set_ylim(-0.0, 0.4)

    axes.set_axisbelow(True)
    axes.yaxis.grid(color='white', linestyle='solid')
    axes.set_facecolor((211.0/255,211.0/255,211.0/255))
    axes.yaxis.set_label_coords(0.12,1.02)
    axes.spines['left'].set_color('#dddddd')
    axes.spines['right'].set_color('#dddddd')
    axes.spines['top'].set_color('#dddddd')

    # Peak performance
    #plt.axhline(y=4, linestyle='--', color='black', label='Compute roof')


def benchmark(dirpath):
    data_1 = {}       # "RUN_EM" : { x : [10, 15, 20], y : [3.2, 4.5, 6.7] }
    data_2 = {}       # "RUN_EM" : { x : [10, 15, 20], y : [3.2, 4.5, 6.7] }

    _, axes = plt.subplots()

    
    regex = re.compile(r'\d+')
    for filename in listdir(dirpath):
        if filename.startswith("fast") or filename.startswith("slow"):
            f = open(join(dirpath, filename), "r")
            K, N = map(int, re.findall(regex, filename))
            if filename[0] == 'f':
                read_one_output(f, N, K, data_1)
            else:
                read_one_output(f, N, K, data_2)
    
    set_up_perf_plot(axes)

    plot_line(plt, data_1, color='blue')
    plot_line(plt, data_2, color='red')
    plt.show()

if __name__ == '__main__':
    benchmark(sys.argv[1])
    

