import matplotlib.pyplot as plt
from benchmarking import run_em
from benchmarking import read_one_output
import sys
import re
import os
from os.path import join


X_MAX_LIM = 2**(8)
X_MIN_LIM = 2**(-8)
Y_MIN_LIM = 2**(-8)

def plot_perf_roof(pi, beta, name, axes):
	roof_pi_y = [pi, pi]
	roof_pi_x = [X_MIN_LIM, X_MAX_LIM]
	axes.plot(roof_pi_x, roof_pi_y, color=(0.7, 0.1, 0.1), linestyle="solid", linewidth=1)
	# We want to offset the text by a small amount above.
	# But since we're in log-log world, we must scale this offset by how high we
	# are so that it's the same for all lines.
	axes.text(5, pi + (pi * 0.2), name)


def plot_roofline(operational_intensity, performance, precision):

	fig, axes = plt.subplots()

	# Pi_no_vec, pi_vec
	if (precision == 'd'):
		pi = [2, 8]
		names = ['$π_{scalar}$', '$π_{vector}$']
	elif (precision == 'f'):
		pi = [2, 16]
		names = ['$π_{scalar}$', '$π_{vector}$']
	else:
		print("I do not know what precision you are talking about.\n")
		usage_and_quit()

	beta = 34.1 / 3.4

    #Computational roof
	for i, p in enumerate(pi):
		plot_perf_roof(p, beta, names[i], axes)

	#Memory roof
	beta_line_x = [p / beta for p in pi]
	beta_line_x.append(X_MIN_LIM)
	beta_line_y = pi
	beta_line_y.append(X_MIN_LIM * beta)

	axes.loglog(beta_line_x, beta_line_y, basex=2, basey=2,color=(0.1, 0.1, 0.7), linewidth=1)

	#Plot our op intensity
	plt.plot(operational_intensity, performance, color='black', marker="o", markersize=4, linewidth=0.85, antialiased=True)


	# axes.set_title('Roofline Model', loc="left", y=1.08)
	axes.set_xlabel('Operational Intensity [Flops/Byte]')
	axes.set_axisbelow(True)
	axes.yaxis.grid(color='white', linestyle='solid')
	axes.set_facecolor((211.0/255,211.0/255,211.0/255))
	axes.set_ylim(X_MIN_LIM, X_MAX_LIM)
	axes.set_xlim(X_MIN_LIM, X_MAX_LIM)

	plt.ylabel('Performance [GFlops/s]',rotation="0")
	axes.yaxis.set_label_coords(0.09,1.02)
	axes.spines['left'].set_color('#dddddd')
	axes.spines['right'].set_color('#dddddd')
	axes.spines['top'].set_color('#dddddd')


	# plt.text(8,4.5,'peak performance 1',rotation=0, color = "black")
	# plt.text(8,17,'peak performance 2',rotation=0, color = "black")

	# plt.text(1/2,8.2,'memory bandwidth',rotation=37, color = "black")
	# plt.text(100,0.2,'L2',rotation=0, color = "black")
	# plt.text(550,0.2,'L3',rotation=0, color = "black")
	fig.savefig('./profiling/Roofline.png')
	plt.show()



def parse_perf_files(performance_path, timings_path):
	operational_intensity = []
	memory_reads = []
	memory_writes = []
	flop_count = []
	num_docs = []

	#Get the memory transfers from perf files
	regex = re.compile(r'\d+')
	for filename in os.listdir(performance_path):
		if "perf" in filename:
			K, N = map(int, re.findall(regex, filename))
			num_docs.append(N)
			for line in open(performance_path + "/" + filename):
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

	num_docs, bytes_transfers = (zip(*sorted(zip(num_docs, bytes_transfers))))

	#Get the flop count and the performance from the timings
	data = {}
	for filename in os.listdir(timings_path):
            if("timings" in filename):
                f = open(join(timings_path, filename), "r")
                # Extract K and N from the filename
                K, N = map(int, re.findall(regex, filename))
                if N in num_docs:
                	read_one_output(f, N, K, data)
                	flop_count[ num_docs.index(N) ] = run_em(N, K).full()
                	pass
                pass
            pass

	operational_intensity = [x / y for x, y in zip(flop_count, bytes_transfers)]


	plt_op = []
	plt_perf = []
	for i in range(len(num_docs)):
		n = num_docs[i]
		if n in data['RUN_EM']['x']:
			assert flop_count[i] is not 0
			plt_op.append(operational_intensity[i])
			plt_perf.append(data['RUN_EM']['y'][ data['RUN_EM']['x'].index(n) ])

	return plt_op, plt_perf

def create_roofline(performance_path, timings_path, precision):

	operational_intensity, performance = parse_perf_files(performance_path, timings_path)
	plot_roofline(operational_intensity, performance, precision)

def usage_and_quit():
    print("\nCreating the roofline plot.")
    print("\nUsage: python roofline.py perf_dir d/f")
    sys.exit()

if __name__ == '__main__':
    if str(sys.argv[1]) in {"-h", "--help"}:
        usage_and_quit()
    if len(sys.argv) != 4:
        print('Wrong number of arguments')
        usage_and_quit()

    create_roofline(str(sys.argv[1]), str(sys.argv[2]), str(sys.argv[3]))


plt.show()
