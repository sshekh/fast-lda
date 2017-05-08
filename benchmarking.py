#! /usr/bin/python
import numpy as np
import sys
import matplotlib.pyplot as plt

flops = { "RUN_EM" : 1., "LDA_INFERENCE" : 1., "DIGAMMA" : 1., "LOG_SUM" : 1.,
        "LOG_GAMMA" : 1., "DOC_E_STEP" : 1., "LIKELIHOOD" : 1., "EM_CONVERGE" : 1,
        "INFERENCE_CONVERGE" : 1.
        }

def read_one_output(f, n, dict)
    header = f.readline().split(',')
    assert(header[0] == 'Accumulator'
       and header[1] == ' Total count'
       and header[2] == ' Average count')
    global data
    for i in range(9):
        s = f.readline().split(',')
        fn = s[0].strip()
        if not fn in dict: dict[fn] = { 'x' : [], 'y' : []}
        dict[fn]['x'].append(n)
        dict[fn]['y'].append(flops[fn] / float(s[2]))
        pass
    pass

def benchmark(n_list):
    data_1 = {}       # "RUN_EM" : { x : [10, 15, 20], y : [3.2, 4.5, 6.7] }
    data_2 = {}       # "RUN_EM" : { x : [10, 15, 20], y : [3.2, 4.5, 6.7] }

    for n in n_list:

        f1 = open("profiling/fast_timings" + str(n) + ".csv", "r")
        f2 = open("profiling/fast_timings" + str(n) + ".csv", "r")
        
        read_one_output(f1, n, data_1)
        read_one_output(f2, n, data_2)

        # plot roof
        plt.axhline(y=4, linestyle='--', color='black', label='Compute roof')
        plt.ylabel('performance (flops/cycle)')
        plt.xlabel('number of documents')
        
        for tp, vals in dict_1:
            plt.plot(vals['x'], vals['y'], '^-', color='blue', linewidth=2)
            #plt.text(, , tp)        # label 
        for tp, vals in dict_2:
            plt.plot(vals['x'], vals['y'], '^-', color='red', linewidth=2)
            #plt.text(, , tp)        # label 

        pass


if __name__ == '__main__':
    benchmark(sys.argv[1], sys.argv[2])
    

