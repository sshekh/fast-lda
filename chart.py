#! /usr/bin/python
import numpy as np
import sys
import matplotlib.pyplot as plt


D = 135
# A little bird told us that this is the proper value
V = 10473

iters = {"EM_CONVERGE" : 0, "INFERENCE_CONVERGE" : 0, "ALPHA_CONVERGE" : 0}  # conv counts for var iterations 

fns = ["RUN_EM", "LDA_INFERENCE", "DIGAMMA", "LOG_SUM", "DOC_E_STEP", "LIKELIHOOD", "MLE", "OPT_ALPHA", "TRIGAMMA", "LOG_GAMMA"]

def memoize(f):
    cache = {}
    return lambda *args: cache[args] if args in cache else cache.update({args: f(*args)}) or cache[args]

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
            0 + 0 + 2 * avg_flops["LOG_GAMMA"] + \
            0 + 2 * avg_flops["DIGAMMA"] + 0 + 2 * avg_flops["TRIGAMMA"]) + \
            0 # poor lonely exp

def mle(N, K):
    return N * K * 0 + avg_flops["OPT_ALPHA"]

def likelihood(N, K):
    return K * avg_flops["DIGAMMA"] + 0 + avg_flops["DIGAMMA"] + 0 + 3 * avg_flops["LOG_GAMMA"] + \
            K * 0 + K * D * 6 * 0

def lda_inference(N, K):
    return K * 0 + K * avg_flops["DIGAMMA"] + K * D * 0 + \
        iters["INFERENCE_CONVERGE"] * (D * K * (avg_flops["DIGAMMA"] + 0 + avg_flops["LOG_SUM"]) + \
            avg_flops["LIKELIHOOD"] + 0)

def doc_e_step(N, K):
    return avg_flops["LDA_INFERENCE"] + K * 0 + 0 + avg_flops["DIGAMMA"] + K * N * 0

def run_em(N, K):
    return 0 + avg_flops["MLE"] + iters["EM_CONVERGE"] * (N * avg_flops["DOC_E_STEP"] + 0 + \
            avg_flops["MLE"] + 0)

impurity = { "RUN_EM" : run_em, "LDA_INFERENCE" : lda_inference, "DIGAMMA" : digamma, "LOG_SUM" : log_sum,
        "LOG_GAMMA" : log_gamma, "TRIGAMMA" : trigamma, "DOC_E_STEP" : doc_e_step, "LIKELIHOOD" : likelihood, "MLE" : mle, "OPT_ALPHA" : opt_alpha}

tot_flops = {"RUN_EM" : 0., "LDA_INFERENCE" : 0., "DIGAMMA" : 0., "LOG_SUM" : 0., "DOC_E_STEP" : 0., "LIKELIHOOD" : 0., "MLE" : 0., \
        "OPT_ALPHA" : 0., "TRIGAMMA" : 0., "LOG_GAMMA" : 0.}
pur_flops = {"RUN_EM" : 0., "LDA_INFERENCE" : 0., "DIGAMMA" : 0., "LOG_SUM" : 0., "DOC_E_STEP" : 0., "LIKELIHOOD" : 0., "MLE" : 0., \
        "OPT_ALPHA" : 0., "TRIGAMMA" : 0., "LOG_GAMMA" : 0.}
avg_flops = {"RUN_EM" : 0., "LDA_INFERENCE" : 0., "DIGAMMA" : 0., "LOG_SUM" : 0., "DOC_E_STEP" : 0., "LIKELIHOOD" : 0., "MLE" : 0., \
        "OPT_ALPHA" : 0., "TRIGAMMA" : 0., "LOG_GAMMA" : 0.}

@memoize
def read_one_output(fname, K, N):
    f = open(fname, "r")
    header = f.readline().split(',')
    header = [h.strip() for h in header]
    assert header[0] == 'Accumulator' \
        and header[1] == 'Total count' \
       and header[2] == 'Average count', "Output file not in proper format"
    lines = f.readlines()

    for fn in fns:
        tot_flops[fn] = 0.
        pur_flops[fn] = 0.
        avg_flops[fn] = 0.
        pass
    for itr in iters: iters[itr] = 0.

    for i in range(len(lines)):
        s = lines[i].split(',')
        fn = s[0].strip()
        if 'CONVERGE' not in fn:
            if s[2].strip() is not '0':     # code inside function not called case
                tot_cnt = float(s[1])
                avg_cnt = float(s[2])
                tot_flops[fn] = tot_cnt
                avg_flops[fn] = avg_cnt
        else:
            iters[fn] = float(s[2])
        pass

    # purifying
    pur_flop_list = [0] * len(fns)
    for i, fn in enumerate(fns):
        pur_flops[fn] = tot_flops[fn] - impurity[fn](N, K)
        print("flops ", fn, tot_flops[fn], pur_flops[fn])
        pur_flop_list[i] = pur_flops[fn]
    pass
    return pur_flop_list 

colors = { "RUN_EM" : "green", "LDA_INFERENCE" : "blue", "DIGAMMA" : "black", "LOG_SUM" : "purple",
        "LOG_GAMMA" : "purple", "TRIGAMMA" : "cyan", "DOC_E_STEP" : "orange", "LIKELIHOOD" : "red", "MLE" : "cyan", "OPT_ALPHA" : "cyan"}       

def stacked_bar_plot(filenames, KNs, xticklabels=None):
    flops = []
    for i, f in enumerate(filenames):
        flps = read_one_output(f, KNs[i][0], KNs[i][1])
        flops.append(flps)
        pass
    flops = np.array(flops)
    flops = np.transpose(flops)

    width = 1
    ind = np.arange(len(filenames))
    bottom = np.zeros(len(ind))
    fig, ax = plt.subplots()
    p = [None] * len(fns)
    for i in range(len(fns)):
        p[i] = ax.bar(ind, flops[i], width, bottom=bottom, color=colors[fns[i]])
        bottom = np.sum([bottom, flops[i]], axis=0)
    
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
        flops = read_one_output(filenames[i], KNs[i][0], KNs[i][1]) 
        p[i] = ax.bar(ind + i * width, flops, width, color = colors2[i])

    ax.set_ylabel('flop count')
    ax.set_title('Flop counts for different runs per group')
    ax.set_xticks(ind)
    ax.set_xticklabels(fns)
    ax.set_yscale("log")
    if legends is not None:
        plt.legend(p, legends)
    ax.grid(linestyle='--', linewidth=2, axis='y')
    plt.show()


if __name__ == "__main__":
    if len(sys.argv) <= 1 or ((len(sys.argv) - 1) % 4 is not 0):
        print("python chart.py -f|s folder1 k1 n1 <-f|s folder2 k2 n2>...")
        sys.exit(1)
    filenames = []
    KNs = []
    legends = []
    for i in range(1, len(sys.argv), 4):
        mode = "fast"
        if sys.argv[i] == '-s': mode = 'slow'
        filename = sys.argv[i + 1] + "/" + mode + "_timings_" + sys.argv[i + 2] + "_" + sys.argv[i + 3] + ".csv"
        filenames.append(filename)
        KNs.append((int(sys.argv[i + 2]), int(sys.argv[i + 3])))
        legend = mode + "_" + sys.argv[i + 2] + "_" + sys.argv[i + 3]
        legends.append(legend)
        pass

    bar_plot(filenames, KNs, legends)
    stacked_bar_plot(filenames, KNs, legends)
