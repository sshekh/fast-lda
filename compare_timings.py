import sys
import numpy as np
import matplotlib.pyplot as plt
import re
from os import listdir
from os.path import join

import benchmarking
import pltutils
from pltutils import fns

def read_one_timer_all_Ns(path, timer_name, label="", xaxis='N'):
    data = {'x' : [], 'y' : []}
    pltutils.set_corpus_stats(path)
    for filename in listdir(path):
            if("timings" in filename):
                fname = join(path, filename)
                K, N, _, _, perf = pltutils.read_one_output(fname)
                data['x'].append(K if xaxis == 'K' else N)
                data['y'].append(perf[fns.index(timer_name)])

    # amend the label afterwards so we can obtain the automated cycle count for the timer
    return {label + " " + timer_name : data}


def compare_timings(path1, path2, timer_name, xaxis='N'):
    """
    path1           directory with old timings       
    path2           directory with new timings
    timer_name      has to match the name of a timer in the timing files
    """
    data_1 = read_one_timer_all_Ns(path1, timer_name, "old", xaxis)    
    data_2 = read_one_timer_all_Ns(path2, timer_name, "new", xaxis)

    _, axes = plt.subplots()
    benchmarking.set_up_perf_plot(axes, xaxis)

    benchmarking.plot_line(plt, data_1, color="red", label_offset=0.01)
    benchmarking.plot_line(plt, data_2, color="green", label_offset=0.01)
    plt.show()

def usage_and_quit():
    print("\nCompare performance of one function for two batches of timings")
    print("\nUsage: python compare_timings.py old_path new_path2 timer_name N|K")
    print()
    sys.exit()

if __name__ == '__main__':
    if str(sys.argv[1]) in {"-h", "--help"}:
        usage_and_quit()
    if len(sys.argv) != 5:
        print('Wrong number of arguments')
        usage_and_quit()

    compare_timings(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
