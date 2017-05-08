#! /usr/bin/python
import numpy as np
import sys
import matplotlib.pyplot as plt
from os import listdir
from os.path import join
import re

EPS = 1e-12     # Average count is float and integer count is a text in case of 0 calls

flops = { "RUN_EM" : 1., "LDA_INFERENCE" : 1., "DIGAMMA" : 1., "LOG_SUM" : 1.,
        "LOG_GAMMA" : 1., "DOC_E_STEP" : 1., "LIKELIHOOD" : 1., "EM_CONVERGE" : 1,
        "INFERENCE_CONVERGE" : 1.
        }   # EM_CONVERGENCE and INFERENCE_CONVERGE are # iteration counts for convergence

def plot(plt, data, color):
    for tp, vals in data.items():
        if len(vals['x']) is not 0:
            x, y = (zip(*sorted(zip(vals['x'], vals['y']))))
            plt.plot(x, y, '^-', color=color, linewidth=2)
            plt.text(x[0], y[0] + 0.002 , tp)        # label 

def read_one_output(f, n, dict):
    header = f.readline().split(',')
    header = [h.strip() for h in header]
    assert header[0] == 'Accumulator' \
        and header[1] == 'Total count' \
       and header[2] == 'Average count', "Output file not in proper format"
    for i in range(7):
        s = f.readline().split(',')
        fn = s[0].strip()
        if not fn in dict: dict[fn] = { 'x' : [], 'y' : []}
        avg_cnt = float(s[2])
        if abs(avg_cnt) > EPS:
            dict[fn]['x'].append(n)
            dict[fn]['y'].append(flops[fn] / avg_cnt)
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
            n = int(regex.search(filename).group(0))
            if filename[0] == 'f':
                read_one_output(f, n, data_1)
            else:
                read_one_output(f, n, data_2)

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
    

