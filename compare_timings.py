import sys
import numpy as np
import matplotlib.pyplot as plt
import re
from os import listdir
from os.path import join

import benchmarking

def read_one_timer_all_Ns(path, timer_name, label=""):
    data = {}
    regex = re.compile(r'\d+')
    for filename in listdir(path):
            if("timings" in filename):
                f = open(join(path, filename), "r")
                # Extract K and N from the filename
                K, N = map(int, re.findall(regex, filename))
                benchmarking.read_one_output(f, N, K, data)

    # amend the label afterwards so we can obtain the automated cycle count for the timer
    return {label + " " + timer_name : data[timer_name]}


def compare_timings(path1, path2, timer_name):
    """
    path1           directory with old timings       
    path2           directory with new timings
    timer_name      has to match the name of a timer in the timing files
    """
    data_1 = read_one_timer_all_Ns(path1, timer_name, "old")    
    data_2 = read_one_timer_all_Ns(path2, timer_name, "new")

    _, axes = plt.subplots()
    benchmarking.set_up_perf_plot(axes)

    benchmarking.plot_line(plt, data_1, color="red", label_offset=0.01)
    benchmarking.plot_line(plt, data_2, color="green", label_offset=0.01)
    plt.show()

def usage_and_quit():
    print("\nCompare runtime of one function for two batches of timings")
    print("\nUsage: python compare_timings.py old_path new_path2 timer_name")
    print()
    sys.exit()

if __name__ == '__main__':
    if str(sys.argv[1]) in {"-h", "--help"}:
        usage_and_quit()
    if len(sys.argv) != 4:
        print('Wrong number of arguments')
        usage_and_quit()

    compare_timings(str(sys.argv[1]), str(sys.argv[2]), str(sys.argv[3]))