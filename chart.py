#! /usr/bin/python
import numpy as np
import sys
import matplotlib.pyplot as plt
import os.path
import re
import benchmarking

PURIFY = False

D = None
V = None
EPS = 1e-8

iters = {"EM_CONVERGE" : 0, "INFERENCE_CONVERGE" : 0, "ALPHA_CONVERGE" : 0}  # conv counts for var iterations

fns = ["RUN_EM", "LDA_INFERENCE", "DIGAMMA", "LOG_SUM", "DOC_E_STEP", "LIKELIHOOD", "MLE", "OPT_ALPHA", "TRIGAMMA", "LOG_GAMMA"]

def digamma(N, K):
    return 0

def log_sum(N, K):
    return 0

def log_gamma(N, K):
    return 0

def trigamma(N, K):
    return 0 + 6 * 0

def random_initialize_ss(N, K):
    return K * N * 0

def opt_alpha(N, K):
    return iters["ALPHA_CONVERGE"] * ( \
            0 + 0 + 2 * avg_cycles["LOG_GAMMA"] + \
            0 + 2 * avg_cycles["DIGAMMA"] + 0 + 2 * avg_cycles["TRIGAMMA"]) + \
            0 # poor lonely exp

def mle(N, K):
    return N * K * 0 + avg_cycles["OPT_ALPHA"]

def likelihood(N, K):
    return K * avg_cycles["DIGAMMA"] + 0 + avg_cycles["DIGAMMA"] + 0 + 3 * avg_cycles["LOG_GAMMA"] + \
            K * 0 + K * D * 6 * 0

def lda_inference(N, K):
    return K * 0 + K * avg_cycles["DIGAMMA"] + K * D * 0 + \
        iters["INFERENCE_CONVERGE"] * (D * K * (avg_cycles["DIGAMMA"] + 0 + avg_cycles["LOG_SUM"]) + \
            avg_cycles["LIKELIHOOD"] + 0)

def doc_e_step(N, K):
    return avg_cycles["LDA_INFERENCE"] + K * 0 + 0 + avg_cycles["DIGAMMA"] + K * N * 0

def run_em(N, K):
    return 0 + avg_cycles["MLE"] + iters["EM_CONVERGE"] * (N * avg_cycles["DOC_E_STEP"] + 0 + \
            avg_cycles["MLE"] + 0)

impurity = { "RUN_EM" : run_em, "LDA_INFERENCE" : lda_inference, "DIGAMMA" : digamma, "LOG_SUM" : log_sum,
        "LOG_GAMMA" : log_gamma, "TRIGAMMA" : trigamma, "DOC_E_STEP" : doc_e_step, "LIKELIHOOD" : likelihood, "MLE" : mle, "OPT_ALPHA" : opt_alpha}

tot_cycles = {"RUN_EM" : 0., "LDA_INFERENCE" : 0., "DIGAMMA" : 0., "LOG_SUM" : 0., "DOC_E_STEP" : 0., "LIKELIHOOD" : 0., "MLE" : 0., \
        "OPT_ALPHA" : 0., "TRIGAMMA" : 0., "LOG_GAMMA" : 0.}
pur_cycles = {"RUN_EM" : 0., "LDA_INFERENCE" : 0., "DIGAMMA" : 0., "LOG_SUM" : 0., "DOC_E_STEP" : 0., "LIKELIHOOD" : 0., "MLE" : 0., \
        "OPT_ALPHA" : 0., "TRIGAMMA" : 0., "LOG_GAMMA" : 0.}
avg_cycles = {"RUN_EM" : 0., "LDA_INFERENCE" : 0., "DIGAMMA" : 0., "LOG_SUM" : 0., "DOC_E_STEP" : 0., "LIKELIHOOD" : 0., "MLE" : 0., \
        "OPT_ALPHA" : 0., "TRIGAMMA" : 0., "LOG_GAMMA" : 0.}

def set_corpus_stats(location):
    global D
    global V

    with open(location + '/info.txt') as f:
        found = False
        ln = 'This is the line'
        while ln != '':
            ln = f.readline()
            if ln.startswith('use_long'):
                found = True
                # A little bird told us that these were the proper values
                if ln.endswith('True\n'): # Long corpus
                    D = 11002
                    V = 48613
                else: # Regular corpus
                    D = 135
                    V = 10473
                break

        if not found:
            print('Warning: use_long setting not found, assuming ap corpus...')
            D = 135
            V = 10473


def read_one_output(fname, K, N):
    if D is None:
        set_corpus_stats(os.path.dirname(fname) or '.')
        benchmarking.set_corpus_stats(os.path.dirname(fname) or '.')

    f = open(fname, "r")
    header = f.readline().split(',')
    header = [h.strip() for h in header]
    assert header[0] == 'Accumulator' \
        and header[1] == 'Total count' \
       and header[2] == 'Average count', "Output file not in proper format"
    lines = f.readlines()

    for fn in fns:
        tot_cycles[fn] = 0.
        pur_cycles[fn] = 0.
        avg_cycles[fn] = 0.
        pass
    for itr in iters: iters[itr] = 0.

    for i in range(len(lines)):
        s = lines[i].split(',')
        fn = s[0].strip()
        if 'CONVERGE' not in fn:
            if s[2].strip() is not '0':     # code inside function not called case
                tot_cnt = float(s[1])
                avg_cnt = float(s[2])
                tot_cycles[fn] = tot_cnt
                avg_cycles[fn] = avg_cnt
        else:
            iters[fn] = float(s[2])
        pass

    ret_flop_list = [0] * len(fns)
    if PURIFY:
        # purifying
        for i, fn in enumerate(fns):
            pur_cycles[fn] = tot_cycles[fn] - impurity[fn](N, K)
            print("cycles ", fn, tot_cycles[fn], pur_cycles[fn])
            ret_flop_list[i] = pur_cycles[fn]
    else:
        for i, fn in enumerate(fns):
            ret_flop_list[i] = tot_cycles[fn]

    return ret_flop_list

colors = { "RUN_EM" : "green", "LDA_INFERENCE" : "blue", "DIGAMMA" : "black", "LOG_SUM" : "purple",
        "LOG_GAMMA" : "purple", "TRIGAMMA" : "cyan", "DOC_E_STEP" : "orange", "LIKELIHOOD" : "red", "MLE" : "cyan", "OPT_ALPHA" : "cyan"}

def stacked_bar_plot(filenames, KNs, xticklabels=None):
    cycles = []
    for i, f in enumerate(filenames):
        cls = read_one_output(f, KNs[i][0], KNs[i][1])
        cycles.append(cls)
        pass
    cycles = np.array(cycles)
    cycles = np.transpose(cycles)

    width = 1
    ind = np.arange(len(filenames))
    bottom = np.zeros(len(ind))
    fig, ax = plt.subplots()
    p = [None] * len(fns)
    for i in range(len(fns)):
        p[i] = ax.bar(ind, cycles[i], width, bottom=bottom, color=colors[fns[i]])
        bottom = np.sum([bottom, cycles[i]], axis=0)

    plt.legend(p, fns)
    if xticklabels is not None:
        ax.set_xticklabels(xticklabels)

    ax.set_ylabel('flop count')
    ax.set_title('Flop counts for different runs per group')
    ax.set_xticks(ind)
    plt.show()


colors2 = ['red', 'blue', 'green', 'yellow', 'cyan']
def bar_plot(filenames, KNs, legends=None):
    width = 1 / (len(filenames) + 1.)   # width of bars
    ind = np.arange(len(fns))
    fig, ax = plt.subplots()
    p = [None] * len(filenames)
    for i in range(len(filenames)):
        cycles = read_one_output(filenames[i], KNs[i][0], KNs[i][1])
        p[i] = ax.bar(ind + i * width, cycles, width, color = colors2[i])

    ax.set_ylabel('flop count')
    ax.set_title('Flop counts for different runs per group')
    ax.set_xticks(ind)
    ax.set_xticklabels(fns)
    ax.set_yscale("log")
    if legends is not None:
        plt.legend(p, legends)
    ax.grid(linestyle='--', linewidth=2, axis='y')
    plt.show()

def perf_plot(filenames, KNs, legends=None):
    width = 1 / (len(filenames) + 1.)   # width of bars
    ind = np.arange(len(fns))
    fig, ax = plt.subplots()
    p = [None] * len(filenames)
    for i, filename in enumerate(filenames):
        read_one_output(filename, KNs[i][0], KNs[i][1])
        benchmarking.iters = iters
        perf = [0] * len(fns)
        for j,fn in enumerate(fns):
            if abs(avg_cycles[fn]) > EPS:
                #print(fn, "Full flops", benchmarking.flops[fn](KNs[i][1], KNs[i][0]))
                perf[j] = benchmarking.flops[fn](KNs[i][1], KNs[i][0]).full() / avg_cycles[fn]
        p[i] = ax.bar(ind + i * width, perf, width, color = colors2[i])
    ax.set_ylabel('performance')
    ax.set_title('performance for different runs per group')
    ax.set_xticks(ind)
    ax.set_xticklabels(fns)
    if legends is not None:
        plt.legend(p, legends)
    ax.grid(linestyle='--', linewidth=2, axis='y')
    plt.show()


if __name__ == "__main__":
    if len(sys.argv) == 1: 
        print("python chart.py <filenames list...>")
        sys.exit(1)
    filenames = sys.argv[1:]
    KNs = []
    legends = []
    
    regex = re.compile(r'\d+')
    for filename in filenames:
        K, N = map(int, re.findall(regex, filename))
        KNs.append((K, N))
        pname = filename[:-1]
        something = pname.split('.')[0].split('_')
        legends.append(something[0] + " " + something[2] + " " + something[3])

    bar_plot(filenames, KNs, legends)
    perf_plot(filenames, KNs, legends)
    stacked_bar_plot(filenames, KNs, legends)
