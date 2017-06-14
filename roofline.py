import matplotlib.pyplot as plt
import pltutils
from pltutils import fns
import sys
import re
import os
from os.path import join


X_MAX_LIM = 2**(4)
X_MIN_LIM = 2**(-3)
Y_MIN_LIM = 2**(-4)
Y_MAX_LIM = 2**(5)

class Run:
    def __init__(self, opints, perfs, nums_docs, label):
        '''
        A run is a list of executions of lda. Each execution has a given operational
        intensity, performance and number of documents (for now we assume that
        the number of topics is constant within a run).
        '''
        self.opints = opints
        self.perfs = perfs
        self.nums_docs = nums_docs
        self.label = label

def create_roofline(paths):
    runs = [parse_perf_files(p) for p in paths]

    fig, axes = plt.subplots()
    make_axes(axes)
    plot_roofs(axes, 'd') # Fred: we'll see later if we want to change this

    # colors = [(0.8, 0.0, 0.0), (0.0, 0.8, 0.0), (0.0, 0.0, 0.8),
    #           (0.8, 0.8, 0.0), (0.0, 0.8, 0.8), (0.8, 0.0, 0.8),
    #           (0.8, 0.8, 0.8)]

    colors = ["darkred", "green", "orangered", "blue", "deeppink", "darkviolet"]
    # axes.set_xticks(ind + width)
    extra_label_offsets = [(1/4,1.3), (1,1), (1/8,1.4), (1/11,0.7), (1/10,1), (1/12,1.6)]
    for run, col, offsets in zip(runs, colors, extra_label_offsets):
        plot_run(run, col, offsets)

    plt.show()

def make_axes(axes):
    axes.tick_params(labelsize=24)
    axes.set_xlabel('Operational Intensity [Flops/Byte]', size=28)
    axes.set_axisbelow(True)
    axes.yaxis.grid(color='white', linestyle='solid')
    axes.xaxis.grid(color='white', linestyle='solid')
    axes.set_facecolor((211.0/255,211.0/255,211.0/255))
    axes.set_ylim(Y_MIN_LIM, Y_MAX_LIM)
    #axes.set_xlim(X_MIN_LIM, X_MAX_LIM)

    plt.ylabel('Performance [Flops/Cycle]',rotation="0", size=28)
    axes.yaxis.set_label_coords(0.2,1.02)
    axes.spines['left'].set_color('#dddddd')
    axes.spines['right'].set_color('#dddddd')
    axes.spines['top'].set_color('#dddddd')


def plot_roofs(axes, precision):
    # Pi_no_vec, pi_vec
    if (precision == 'd'):
        pi = [2, 16]
        names = ['$π_{scalar}$', '$π_{vector}$']
    elif (precision == 'f'):
        pi = [2, 32]
        names = ['$π_{scalar}$', '$π_{vector}$']
    else:
        print("I do not know what precision you are talking about.\n")
        usage_and_quit()

    beta = 34.1 / 3.4

    #Computational roofs
    for i, p in enumerate(pi):
        plot_perf_roof(p, beta, names[i], axes)

    #Memory roof
    beta_line_x = [p / beta for p in pi]
    beta_line_x.append(X_MIN_LIM)
    beta_line_y = pi
    beta_line_y.append(X_MIN_LIM * beta)

    axes.loglog(beta_line_x, beta_line_y, basex=2, basey=2,color='black', linewidth=2)

def plot_perf_roof(pi, beta, name, axes):
    roof_pi_y = [pi, pi]
    roof_pi_x = [X_MIN_LIM, X_MAX_LIM]
    roof_compute_x = [pi / beta, X_MAX_LIM]
    #axes.plot(roof_pi_x, roof_pi_y, color='black', linestyle='solid', linewidth=0.8)
    if 'vector' in name:
        axes.plot(roof_compute_x, roof_pi_y, color='black', linestyle='solid', linewidth=2)
        # axes.plot([roof_compute_x[0], roof_compute_x[0]], [pi, Y_MIN_LIM], color='white', linestyle='--', linewidth=1)
    else:
        axes.plot(roof_compute_x, roof_pi_y, color='black', linestyle='solid', linewidth=2)
        # axes.plot([roof_compute_x[0], roof_compute_x[0]], [pi, Y_MIN_LIM], color='white', linestyle='--', linewidth=1)

    # We want to offset the text by a small amount above.
    # But since we're in log-log world, we must scale this offset by how high we
    # are so that it's the same for all lines.
    axes.text(3, pi + (pi * 0.2), name, size=24)


def plot_run(run, col, offsets):

    #print(run.opints)
    #print(run.perfs)

    #Plot our op intensity
    plt.plot(
        run.opints,
        run.perfs,
        color=col,
        marker="o",
        markersize=6,
        linewidth=1.5,
        antialiased=True)

    # # Arrow to the first element
    # plt.annotate(
    #     str(run.nums_docs[0][0]),
    #     xy=(run.opints[0], run.perfs[0]), xytext=(0, -25),
    #     textcoords='offset points', ha='center', va='bottom',
    #     size='medium',
    #     arrowprops=dict(arrowstyle = '-'))

    # # Arrow to the last element
    # plt.annotate(
    #     str(run.nums_docs[-1][0]),
    #     xy=(run.opints[-1], run.perfs[-1]), xytext=(0, -25),
    #     textcoords='offset points', ha='center', va='bottom',
    #     size='medium',
    #     arrowprops=dict(arrowstyle = '-'))

    # Label is to the right of the first element, since in general we go
    # left as we go further in the series
    xlab = run.opints[0] * 1.2 * offsets[0]
    ylab = run.perfs[0] * offsets[1]
    plt.text(xlab, ylab, run.label, color=col, ha='left', va='center', size='22')

def parse_perf_files(dir_path):
    pltutils.set_corpus_stats(dir_path)

    operational_intensity = []
    memory_reads = []
    memory_writes = []
    flop_count = []
    num_docs = []

    with open(dir_path + '/info.txt') as f:
        for ln in f:
            if ln.startswith('Comment: '):
                #Remove beginning and last quote/newline
                comment = ln[len('Comment: "') : -2]
                break

    #Get the memory transfers from perf files
    regex = re.compile(r'\d+')
    for filename in os.listdir(dir_path):
        if "perf" in filename:
            K, N = map(int, re.findall(regex, filename))
            num_docs.append((K, N))
            for line in open(dir_path + "/" + filename):
                if "LLC-load-misses" in line:
                    tokens = line.split()
                    number_parts = tokens[0].split(",")
                    memory_reads.append(float(''.join(number_parts)))
                if "LLC-store-misses" in line:
                    tokens = line.split()
                    number_parts = tokens[0].split(",")
                    memory_writes.append(float(''.join(number_parts)))

    memory_transfers = [x + y for x, y in zip(memory_reads, memory_writes)]

    #Consider the number of bytes transfered as the number of cache misses * the cache line size
    cache_line_size = 64.0
    bytes_transfers = [x * cache_line_size for x in memory_transfers]

    flop_count = [0] * len(bytes_transfers)

    tuples_sorted = sorted(zip(num_docs, bytes_transfers))
    num_docs = [x[0] for x in tuples_sorted]
    bytes_transfers = [x[1] for x in tuples_sorted]

    #Get the flop count and the performance from the timings
    data = {'x' : [], 'y' : []}
    for filename in os.listdir(dir_path):
        if("timings" in filename):
            fullname = join(dir_path, filename)
            # Extract K and N from the filename
            K, N = map(int, re.findall(regex, filename))
            if (K, N) in num_docs:
                k1, n1, flops, _, _, perf = pltutils.read_one_output(fullname)
                assert k1 == K and n1 == N, "Wrong file"
                flop_count[ num_docs.index((K, N)) ] = flops[ fns.index("RUN_EM") ]
                data['x'].append((K,N))
                data['y'].append(perf[ fns.index("RUN_EM") ])
                #print(flop_count)
                pass
            pass
        pass

    operational_intensity = [x / y for x, y in zip(flop_count, bytes_transfers)]

    plt_op = []
    plt_perf = []
    for i in range(len(num_docs)):
        kn = num_docs[i]
        if kn in data['x']:
            idx = data['x'].index(kn)
            assert flop_count[i] is not 0
            plt_op.append(operational_intensity[i])
            plt_perf.append(data['y'][idx])

    return Run(plt_op, plt_perf, num_docs, comment)

def usage_and_quit():
    print("\nCreates a roofline plot, with one line for each timings directory.")
    print("\nUsage: python roofline.py <timings_dir1> [timings_dir2... ]")
    sys.exit()

if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] in {"-h", "--help"}:
        usage_and_quit()

    create_roofline(sys.argv[1:])

