#! /usr/bin/python
import numpy as np
import sys
import matplotlib.pyplot as plt


indices = { "RUN_EM" : 0, "LDA_INFERENCE" : 1, "DIGAMMA" : 2, "LOG_SUM" : 3,
        "LOG_GAMMA" : 9, "TRIGAMMA" : 8, "DOC_E_STEP" : 4, "LIKELIHOOD" : 5, "MLE" : 6, "OPT_ALPHA" : 7}

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

    tot_flops = [0.] * len(indices)
    for i in range(len(lines)):
        s = lines[i].split(',')
        fn = s[0].strip()
        if 'CONVERGE' not in fn:
            if s[2].strip() is not '0':     # code inside function not called case
                total_cnt = float(s[1])
                idx = indices[fn]
                tot_flops[idx] += total_cnt
        pass
    return tot_flops
    pass

colors2 = ['r', 'b', 'g', 'y', 'cyan']
def bar_plot(filenames):
    width = 1 / (len(filenames) + 1.)   # width of bars
    ind = np.arange(len(indices))
    fig, ax = plt.subplots()
    for i in range(len(filenames)): 
        flops = read_one_output(open(filenames[i], "r")) 
        rect = ax.bar(ind + i * width, flops, width, color = colors2[i])

    ax.set_ylabel('flop count')
    ax.set_title('Flop counts for different runs per group')
    ax.set_xticks(ind)
    ax.set_xticklabels(fns)
    ax.set_yscale("log")
    plt.grid(True)
    plt.show()

def stacked_bar_plot(filenames):
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
    p = [None] * len(indices)
    for i in range(len(indices)):
        p[i] = ax.bar(ind, flops[i], width, bottom=bottom, color=colors[fns[i]])
        bottom = np.sum([bottom, flops[i]], axis=0)
    
    plt.legend(p, fns)

    ax.set_ylabel('flop count')
    ax.set_title('Flop counts for different runs per group')
    ax.set_xticks(ind)
    plt.show()



if __name__ == "__main__":
    if (len(sys.argv) - 1) % 4 is not 0:
        print("python chart.py -f|s folder1 k1 n1 <-f|s folder2 k2 n2>...")
        sys.exit(1)
    filenames = []
    for i in range(1, len(sys.argv), 4):
        mode = "fast"
        if sys.argv[i] == '-s': mode = slow
        filename = sys.argv[i + 1] + "/" + mode + "_timings_" + sys.argv[i + 2] + "_" + sys.argv[i + 3] + ".csv"
        filenames.append(filename)
        pass

    #bar_plot(filenames)
    stacked_bar_plot(filenames)
