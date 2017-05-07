#! /usr/bin/python
import numpy as np
import sys
import matplotlib.pyplot as plt

def add_to_plot(idx, x, y, label, color='blue'):
    #plt = plot.figure(idx)
    #plt.grid(true)

    plt.plot(x, y, '^-', color=color, linewidth=2)
    #plt.axvline(x=51.639, linestyle='--', color='black', label='L1')
    #plt.text(51.8, 0.5, 'L1')

    pass

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

def benchmark(filename1, filename2):
    data_1 = {}       # "RUN_EM" : { x : [10, 15, 20], y : [3.2, 4.5, 6.7] }
    data_2 = {}       # "RUN_EM" : { x : [10, 15, 20], y : [3.2, 4.5, 6.7] }

    fp1 = open(filename1, "r")
    fp2 = open(filename2, "r")

    while True:
        n1 = fp1.readline()
        n2 = fp2.readline()

        if n1 is not None and n2 is not None:
            n1 = int(n1)
            n2 = int(n2)
            assert(n1 == n2)
            read_one_output(fp1, n1, data_1)
            read_one_output(fp2, n2, data_2)
        elif n1 is not None or n2 is not None:
            print("Unequal number of runs")
            sys.exit(1)
        else:
            # EOF
            break
    
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
    

