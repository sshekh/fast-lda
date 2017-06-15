import numpy as np
import matplotlib.pyplot as plt
import sys
from os import listdir
from os.path import join
from os.path import dirname
import pltutils
from pltutils import fns
import os

colors = { "RUN_EM" : "green", "LDA_INFERENCE" : "blue", "DIGAMMA" : "black", "LOG_SUM" : "purple",
        "LOG_GAMMA" : "purple", "TRIGAMMA" : "cyan", "DOC_E_STEP" : "orange", "LIKELIHOOD" : "red", "MLE" : "#5E6382", "OPT_ALPHA" : "#00bcd4"}

def stacked_bar_plot(filenames, vecs, xticklabels=None):
    cycles = []
    for i, filename in enumerate(filenames):
        pltutils.set_corpus_stats(dirname(filename))
        _, _, _, cls, _, _ = pltutils.read_one_output(filename, vec=vecs[i])
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

def pie_chart(filenames, vecs, xticklabels=None):
    if len(filenames) > 1:
        print("Pie chart can only be done for one file")
        sys.exit()
    cycles = []
    pltutils.set_corpus_stats(dirname(filenames[0]))
    _, _, _, cls, _, _ = pltutils.read_one_output(filenames[0], vec=vecs[0])
    cycles = cls
    # cycles = np.array(cycles)
    # cycles = np.transpose(cycles)
    # print(cycles)

    select_cycles = np.array(cycles)
    # Manually pick the functions we want
    select_cycles = select_cycles[np.array([1,2,3,5,6])]
    # print(select_cycles)

    # width = 0.75
    ind = np.arange(len(filenames))
    # bottom = np.zeros(len(ind))
    print(plt.style.available)
    plt.style.use('seaborn-colorblind')
    fig, ax = plt.subplots()

    # p = [None] * len(fns)
    select_fns = np.array(fns)
    select_fns = select_fns[np.array([1,2,3,5,6])]
    # print(select_fns)
        # p[i] = ax.bar(ind, cycles[i], width, bottom=bottom, color=colors[fns[i]])

    ax.pie(select_cycles, explode=None, labels=select_fns, autopct='%1.1f%%',
    pctdistance = 0.7, startangle=45, labeldistance=1.1, textprops={'fontsize': 13})
    ax.axis('equal')  # Equal aspect ratio ensures that pie is drawn as a circle.


        # bottom = np.sum([bottom, cycles[i]], axis=0)

    # plt.legend(p, fns)
    # if xticklabels is not None:
        # ax.set_xticklabels(xticklabels)

    # ax.set_ylabel('cycle component')
    # ax.set_title('Distribution of the total runtime over functions', fontdict={'fontsize': 18})
    # ax.set_xticks(ind)
    

    plt.show()


#colors2 = ["darkred", "green", "darkviolet", "blue", "deeppink", "orangered"]
colors2 = ["#0F007F", "#7867FF", "#BCB3FF", "blue", "deeppink", "orangered"]
#colors2 = ["#EEF0FF", "#667BFF", "#00127F", "blue", "deeppink", "orangered"]
def bar_plot(filenames, vecs, legends=None):
    width = 1 / (len(filenames) + 1.)   # width of bars
    ind = np.arange(len(fns[:7]))
    fig, ax = plt.subplots()
    p = [None] * len(filenames)
    for i, filename in enumerate(filenames):
        pltutils.set_corpus_stats(dirname(filename))
        _, _, _, cycles, _, _ = pltutils.read_one_output(filename, vec=vecs[i])
        p[i] = ax.bar(ind + i * width, cycles[:7], width, color = colors2[i])

    ax.set_ylabel('Runtime [Cycles]',rotation="0", size=28)
    ax.yaxis.set_label_coords(0, 1.03)
    #ax.set_title('performance for different runs per group')
    ax.tick_params(labelsize=24)
    ax.set_facecolor((211.0/255,211.0/255,211.0/255))
    ax.set_xticklabels(fns[:7])
    ax.set_xticks(ind)
    ax.set_yscale("log")
    if legends is not None:
        plt.legend(p, legends, prop={'size':24})
    ax.grid(linestyle='--', linewidth=2, axis='y')
    plt.show()

def avgc_plot(filenames, vecs, legends=None):
    width = 1 / (len(filenames) + 1.)   # width of bars
    ind = np.arange(len(fns))
    fig, ax = plt.subplots()
    p = [None] * len(filenames)
    for i, filename in enumerate(filenames):
        pltutils.set_corpus_stats(dirname(filename))
        _, _, _, _, avg_cycles, _ = pltutils.read_one_output(filename, vec=vecs[i])
        p[i] = ax.bar(ind + i * width, avg_cycles, width, color = colors2[i])

    ax.set_ylabel('avg cycle count')
    ax.set_title('Avg Cycles counts for different runs per group')
    ax.set_xticks(ind)
    ax.set_xticklabels(fns)
    ax.set_yscale("log")
    if legends is not None:
        plt.legend(p, legends)
    ax.grid(linestyle='--', linewidth=2, axis='y')
    plt.show()


# regular perf plot
def perf_plot(filenames, vecs, legends=None):
    width = 1 / (len(filenames) + 1.)   # width of bars
    ind = np.arange(len(fns[:-3]))
    fig, ax = plt.subplots()
    p = [None] * len(filenames)
    #print(fns)
    for i, filename in enumerate(filenames):
        pltutils.set_corpus_stats(dirname(filename))
        _, _, flp, cls, _, perf = pltutils.read_one_output(filename, vec=vecs[i])
        p[i] = ax.bar(ind + i * width, perf[:-3], width, color = colors2[i])

    ax.set_xticks(ind)
    ax.set_xticklabels(fns)
    plt.setp(ax.get_xticklabels(), rotation=30, horizontalalignment='center')

    ax.set_ylabel('Performance [Flops/Cycle]',rotation="0", size=28)
    ax.yaxis.set_label_coords(0.22, 1.03)
    fig.tight_layout()
    # ax.set_title('performance for different runs per group')
    ax.tick_params(axis='x', labelsize=20)
    ax.tick_params(axis='y', labelsize=24)
    ax.set_facecolor((211.0/255,211.0/255,211.0/255))
    

    if legends is not None:
        plt.legend(p, legends, prop={'size':22})
    ax.grid(linestyle='--', linewidth=2, axis='y')
    plt.show()



# for icc vs gcc
# def perf_plot(filenames, vecs, legends=None):
#     width = 1 / (len(filenames) + 1.)   # width of bars
#     ind = np.arange(len(fns[:1]))
#     fig, ax = plt.subplots()
#     p = [None] * len(filenames)
#     for i, filename in enumerate(filenames):
#         pltutils.set_corpus_stats(dirname(filename))
#         _, _, flp, cls, _, perf = pltutils.read_one_output(filename, vec=vecs[i])
#         p[i] = ax.bar(ind + 1.5 * i * width, perf[:1], width, color = colors2[i])
#     ax.set_ylabel('Performance [Flops/Cycle]',rotation="0", size=28)
#     ax.yaxis.set_label_coords(0.1, 1.03)
#     #ax.set_title('performance for different runs per group')
#     ax.tick_params(labelsize=24)
#     ax.set_facecolor((211.0/255,211.0/255,211.0/255))
#     ax.set_xticklabels(legends)
#     ax.set_xticks([ind + 1.5 * i * width for i in range(len(filenames))])
#     #if legends is not None:
#     #    plt.legend(p, legends, prop={'size':24})
#     ax.grid(linestyle='--', linewidth=2, axis='y')
#     plt.show()

def run_comp(filenames, vecs, legends=None):
    ind = np.arange(len(filenames))
    fig, ax = plt.subplots()
    perfs = []
    for i, filename in enumerate(filenames):
        pltutils.set_corpus_stats(dirname(filename))
        _, _, flp, cls, _, perf = pltutils.read_one_output(filename, vec=vecs[i])
        perfs.append(perf[0])
    p = ax.plot(perfs, linestyle='solid', marker='o', linewidth=2, color='deeppink') 
    ax.set_ylabel('Performance [Flops/Cycle]',rotation="0", size=28)
    ax.yaxis.set_label_coords(0.1, 1.03)
    #ax.set_title('performance for different runs per group')
    ax.tick_params(labelsize=24)
    ax.set_facecolor((211.0/255,211.0/255,211.0/255))
    ax.set_xticklabels(legends)
    ax.set_xticks(ind)
    ax.grid(linestyle='--', linewidth=2, axis='y')
    plt.show()

def conv_plot(filenames, vecs, legends=None):
    width = 1 / (len(filenames) + 1.)   # width of bars
    its = list(pltutils.iters.keys())
    ind = np.arange(len(its))
    fig, ax = plt.subplots()
    p = [None] * len(filenames)
    for i, filename in enumerate(filenames):
        pltutils.set_corpus_stats(dirname(filename))
        pltutils.read_one_output(filename, vec=vecs[i])
        convs = [pltutils.iters[it] for it in its]
        p[i] = ax.bar(ind + i * width, convs, width, color = colors2[i])
    ax.set_ylabel('#convergence iters')
    ax.set_title('#convergence iters for different runs per group')
    ax.set_xticks(ind)
    ax.set_xticklabels(its)
    if legends is not None:
        plt.legend(p, legends)
    ax.grid(linestyle='--', linewidth=2, axis='y')
    plt.show()

def usage_and_quit():
    print("\ncompare any number of timing files for #cycles perf and #comp cycles")
    print("\npython chart.py <filenames path ?vec list...>")
    print("\na good convention is to go from slow file to fast file")
    sys.exit(1)

if __name__ == "__main__":
    if str(sys.argv[1]) in {"-h", "--help"} or len(sys.argv) == 1:
        usage_and_quit()
    filenames = []
    legends = []
    vecs = []
    
    for arg in sys.argv[1:]:
        if arg in {"vec", "no-vec"}:
            vecs[-1] = True if arg == "vec" else False
        else:
            filenames.append(arg)
            vecs.append(False)

    for filename in filenames:
        pname = os.path.basename(filename)
        something = pname.split('.')[0].split('_')
        with open(dirname(filename) + '/info.txt') as f:
            for ln in f:
                if ln.startswith('Comment: '):
                    #Remove beginning and last quote/newline
                    comment = ln[len('Comment: "') : -2]
                    break
        legends.append(comment)

    #bar_plot(filenames, vecs, legends)
    #avgc_plot(filenames, vecs, legends)
    #conv_plot(filenames, vecs, legends)
    # perf_plot(filenames, vecs, legends)
    #run_comp(filenames, vecs, legends)
    # stacked_bar_plot(filenames, vecs, legends)
    pie_chart(filenames, vecs, legends)
