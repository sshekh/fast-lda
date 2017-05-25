import numpy as np
import matplotlib.pyplot as plt
import sys
from os import listdir
from os.path import join
from os.path import dirname
import pltutils

colors = { "RUN_EM" : "green", "LDA_INFERENCE" : "blue", "DIGAMMA" : "black", "LOG_SUM" : "purple",
        "LOG_GAMMA" : "purple", "TRIGAMMA" : "cyan", "DOC_E_STEP" : "orange", "LIKELIHOOD" : "red", "MLE" : "#5E6382", "OPT_ALPHA" : "#00bcd4"}

label_offsets = { "RUN_EM" : 0.008, "LDA_INFERENCE" : 0.008, "DIGAMMA" : 0.02, "LOG_SUM" : -0.015,
        "LOG_GAMMA" : 0.008, "TRIGAMMA" : 0.01, "DOC_E_STEP" : -0.01, "LIKELIHOOD" : -0.015, "MLE" : 0.01, "OPT_ALPHA" : 0.01 }

def plot_line(plt, data, color=None, label_offset=None):
    for fn, vals in data.items():
        if len(vals['x']) is not 0:
            x, y = (zip(*sorted(zip(vals['x'], vals['y']))))
            if color is None:
                use_color = colors[fn]
            else:
                use_color = color
            if label_offset is None:
                use_label_offset = label_offsets[fn]
            else:
                use_label_offset = label_offset
            plt.plot(x, y, '^-', color=use_color, linewidth=2)
            plt.text(x[0], y[0] + use_label_offset , fn, color=use_color, size=9)

def set_up_perf_plot(axes, xaxis = 'N'):
    #Per plot settings
    axes.set_title('Performance on Skylake',  y=1.08, loc = "center")
    axes.set_xlabel('$k$ topics' if xaxis == 'K' else '$n$ documents')
    axes.set_ylabel('Performance [flops/cycles]',rotation="0")
    #axes.set_ylim(-0.0, 0.4)

    axes.set_axisbelow(True)
    axes.yaxis.grid(color='white', linestyle='solid')
    axes.set_facecolor((211.0/255,211.0/255,211.0/255))
    axes.yaxis.set_label_coords(0.12,1.02)
    axes.spines['left'].set_color('#dddddd')
    axes.spines['right'].set_color('#dddddd')
    axes.spines['top'].set_color('#dddddd')

    # Peak performance
    #plt.axhline(y=4, linestyle='--', color='black', label='Compute roof')


def benchmark(dirpath, vec=False, xaxis='N'):
    pltutils.set_corpus_stats(dirpath)
    data_1 = {}       # "RUN_EM" : { x : [10, 15, 20], y : [3.2, 4.5, 6.7] }
    data_2 = {}       # "RUN_EM" : { x : [10, 15, 20], y : [3.2, 4.5, 6.7] }

    _, axes = plt.subplots()

    for filename in listdir(dirpath):
        if filename.startswith("fast") or filename.startswith("slow"):
            K, N, _, _, perf = pltutils.read_one_output(join(dirpath, filename), vec=vec)
            dic = data_1 if filename[0] == 'f' else data_2
            for i, fn in enumerate(pltutils.fns):
                if not fn in dic: dic[fn] = {'x' : [], 'y' : []}
                dic[fn]['x'].append(K if xaxis == 'K' else N)  
                dic[fn]['y'].append(perf[i])  

    set_up_perf_plot(axes, xaxis)

    plot_line(plt, data_1)
    plot_line(plt, data_2)
    plt.show()

def usage_and_quit():
    print("\nPerformance of all timers for one batch of runs")
    print("\npython benchmarking.py <dirname> x axis = <N|K> ?vec")
    sys.exit(1)

if __name__ == '__main__':
    if str(sys.argv[1]) in {"-h", "--help"}:
        usage_and_quit()
    if len(sys.argv) < 3:
        print('Wrong number of arguments')
        usage_and_quit()
    vec = False
    if sys.argv[-1] == 'vec':
        vec = True
    benchmark(sys.argv[1], vec, sys.argv[2])


