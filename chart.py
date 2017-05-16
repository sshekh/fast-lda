#! /usr/bin/python
import numpy as np
import sys
import matplotlib.pyplot as plt


colors = { "RUN_EM" : "green", "LDA_INFERENCE" : "blue", "DIGAMMA" : "black", "LOG_SUM" : "purple",
        "LOG_GAMMA" : "purple", "TRIGAMMA" : "cyan", "DOC_E_STEP" : "orange", "LIKELIHOOD" : "red", "MLE" : "cyan", "OPT_ALPHA" : "cyan"}       

fns = ["RUN_EM", "LDA_INFERENCE", "DIGAMMA", "LOG_SUM", "DOC_E_STEP", "LIKELIHOOD", "MLE", "OPT_ALPHA", "TRIGAMMA", "LOG_GAMMA"]

def read_one_output(f):
    header = f.readline().split(',')
    header = [h.strip() for h in header]
    assert header[0] == 'Accumulator' \
        and header[1] == 'Total count' \
       and header[2] == 'Average count', "Output file not in proper format"
    lines = f.readlines()

    tot_flops = [0.] * len(fns)
    for i in range(len(lines)):
        s = lines[i].split(',')
        fn = s[0].strip()
        if 'CONVERGE' not in fn:
            if s[2].strip() is not '0':     # code inside function not called case
                total_cnt = float(s[1])
                idx = fns.index(fn)
                tot_flops[idx] += total_cnt
        pass
    return tot_flops
    pass

def stacked_bar_plot(filenames, xticklabels=None):
    flops = []
    for f in filenames:
        flps = read_one_output(open(f, "r"))
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
def bar_plot(filenames, legends=None):
    width = 1 / (len(filenames) + 1.)   # width of bars
    ind = np.arange(len(fns))
    fig, ax = plt.subplots()
    p = [None] * len(filenames)
    for i in range(len(filenames)): 
        flops = read_one_output(open(filenames[i], "r")) 
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
    legends = []
    for i in range(1, len(sys.argv), 4):
        mode = "fast"
        if sys.argv[i] == '-s': mode = 'slow'
        filename = sys.argv[i + 1] + "/" + mode + "_timings_" + sys.argv[i + 2] + "_" + sys.argv[i + 3] + ".csv"
        filenames.append(filename)
        legend = mode + "_" + sys.argv[i + 3] + "_" + sys.argv[i + 2]
        legends.append(legend)
        pass

    bar_plot(filenames, legends)
    stacked_bar_plot(filenames, legends)
