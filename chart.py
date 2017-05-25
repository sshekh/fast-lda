import numpy as np
import matplotlib.pyplot as plt
import sys
from os import listdir
from os.path import join
from os.path import dirname
import pltutils
from pltutils import fns

colors = { "RUN_EM" : "green", "LDA_INFERENCE" : "blue", "DIGAMMA" : "black", "LOG_SUM" : "purple",
        "LOG_GAMMA" : "purple", "TRIGAMMA" : "cyan", "DOC_E_STEP" : "orange", "LIKELIHOOD" : "red", "MLE" : "#5E6382", "OPT_ALPHA" : "#00bcd4"}

def stacked_bar_plot(filenames, xticklabels=None):
    cycles = []
    for i, filename in enumerate(filenames):
        pltutils.set_corpus_stats(dirname(filename))
        _, _, _, cls, _ = pltutils.read_one_output(filename)
        cycles.append(cls)
        pass
    cycles = np.array(cycles)
    cycles = np.transpose(cycles)

    width = 0.75
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

    ax.set_ylabel('cycle component')
    ax.set_title('#cycle counts for different runs per group')
    ax.set_xticks(ind)
    plt.show()


colors2 = ['red', 'blue', 'orange', 'yellow', 'cyan']
def bar_plot(filenames, legends=None):
    width = 1 / (len(filenames) + 1.)   # width of bars
    ind = np.arange(len(fns))
    fig, ax = plt.subplots()
    p = [None] * len(filenames)
    for i, filename in enumerate(filenames):
        pltutils.set_corpus_stats(dirname(filename))
        _, _, _, cycles, _ = pltutils.read_one_output(filename)
        p[i] = ax.bar(ind + i * width, cycles, width, color = colors2[i])

    ax.set_ylabel('cycle count')
    ax.set_title('Cycles counts for different runs per group')
    ax.set_xticks(ind)
    ax.set_xticklabels(fns)
    ax.set_yscale("log")
    if legends is not None:
        plt.legend(p, legends)
    ax.grid(linestyle='--', linewidth=2, axis='y')
    plt.show()

def perf_plot(filenames, legends=None):
    width = 1 / (len(filenames) + 1.)   # width of bars
    ind = np.arange(len(fns))
    fig, ax = plt.subplots()
    p = [None] * len(filenames)
    #print(fns)
    for i, filename in enumerate(filenames):
        pltutils.set_corpus_stats(dirname(filename))
        _, _, flp, cls, perf = pltutils.read_one_output(filename)
        #print(perf)
        #print(flp)
        #print(cls)
        p[i] = ax.bar(ind + i * width, perf, width, color = colors2[i])
    ax.set_ylabel('performance')
    ax.set_title('performance for different runs per group')
    ax.set_xticks(ind)
    ax.set_xticklabels(fns)
    if legends is not None:
        plt.legend(p, legends)
    ax.grid(linestyle='--', linewidth=2, axis='y')
    plt.show()

def usage_and_quit():
    print("\ncompare any number of timing files for #cycles perf and #comp cycles")
    print("\npython chart.py <filenames full path list...>")
    print("\na good convention is to go from slow file to fast file")
    sys.exit(1)

if __name__ == "__main__":
    if str(sys.argv[1]) in {"-h", "--help"} or len(sys.argv) == 1: 
        usage_and_quit()
    filenames = sys.argv[1:]
    legends = []
    
    for filename in filenames:
        pname = filename.split('/')[-1]
        something = pname.split('.')[0].split('_')
        legends.append(something[0] + " " + something[2] + " " + something[3])

    bar_plot(filenames, legends)
    perf_plot(filenames, legends)
    stacked_bar_plot(filenames, legends)
