'''
We would like to:
    - Generate test data from the slow
    - Test the fast against the slow
    - Benchmark the fast (or the slow)


'''

import sys
import os
import getopt
import subprocess
import time

from colorama import init, Fore, Style

import beta_comp

REFERENCE_FOLDER = 'reference'
REFERENCE_DATA = REFERENCE_FOLDER + '/ref-%d-%d.beta'
LDA_EXE_LOG = './%s-lda/logfiles'
LDA_OUT_BETA = LDA_EXE_LOG + '/final.beta'
LDA_OUT_TIMING = './results/timings.csv'

ALWAYS_GENERATE_REF = False

TIMING_FOLDER = 'timings/%s/'
TIMING_FILENAME = '%s_timings_%d_%d.csv'

RUN_NAME = None

def run_lda(which, k, n):
    params = ['./%s-lda/lda' % which,           # Executable location
            'est',                              # Execution mode (always est)
            str(n),                             # Number of documents
            '1',                                # Initial estimate for alpha
            str(k),                             # Number of topics
            './%s-lda/settings.txt' % which,    # Settings location
            './%s-lda/ap/ap.dat' % which,       # Documents location
            'random',                           # Initialization method
            LDA_EXE_LOG % which]                # Output directory

    print(Fore.LIGHTGREEN_EX)
    print(' ========== ')
    subprocess.call(params)
    print(' ========== ')
    print(Style.RESET_ALL)


def exists(path):
    try:
        os.stat(path)
        return True
    except FileNotFoundError:
        return False

def prompt_yna():
    while True:
        inp = input('(y)es, (n)o, yes to (a)ll\n')
        if inp[0] in {'y', 'n', 'a'}:
            ALWAYS_GENERATE_REF = inp[0] == 'a'
            return inp[0] != 'n'


def generate(k, n):
    # Run the slow LDA, collect the final beta file and move it in the reference.
    print('Generating k=%d n=%d' % (k, n))

    dst = REFERENCE_DATA % (k, n)

    if exists(dst):
        print('Already exists. Skipping...')
        return

    run_lda('slow', k, n)
    os.renames(LDA_OUT_BETA, dst)


def test(k, n):
    # Run the fast LDA and compare the resulting final beta against the reference.
    print('Testing fast against reference k=%d n=%d' % (k, n))
    which = REFERENCE_DATA % (k, n)

    if not exists(which):
        print('Reference file %s not found... ' % which, end='')
        if not ALWAYS_GENERATE_REF:
            print('Do you want to generate it?')
            if not prompt_yna():
                sys.exit(0)

        generate(k, n)

    run_lda('fast', k, n)

    reference = open(which, 'r')
    created = open(LDA_OUT_BETA, 'r')

    print('Comparing against the reference...')
    good = beta_comp.compare(reference, created)

    reference.close()
    created.close()

    if not good:
        # The comparator script has printed the relevant info, just quit.
        sys.exit(1)
    else:
        print('Test %d %d ran successfully' % (k, n))

def bench(k, n, fast, slow):
    which = []

    if fast:
        which.append('fast')
    if slow:
        which.append('slow')

    print('Benchmarking %s k=%d n=%d' % (str(which), k, n))
    for lda in which:
        run_lda(lda, k, n)
        timing_out = (TIMING_FOLDER % RUN_NAME) + (TIMING_FILENAME % (lda, k, n))
        os.rename(LDA_OUT_TIMING, timing_out)

def usage_and_quit():
    print('Fast runner')
    print('')
    print('usage: %s [ gen | test | bench ] options... -- comment' % sys.argv[0])
    print('')
    print('Options: ')
    print('The following two options recieve as argument either one number,\n' +
        'or three separated by a comma (e.g. `--num-docs=50,100,10`. These\n' +
        'three numbers specify the start, end and step values. If a single\n' +
        'number is specified, only that value will be used.' )
    print('\t--num-topics (default: 50)')
    print('\t--num-docs (default: all documents)')
    print('')
    print('Modes:')
    print('\tgen: Generate test data from the slow implementation')
    print('\ttest: Obtain results from the fast implementation, and compare ')
    print('\t  against reference output from the slow implementation.')
    print('\tbench: Check speed of the fast (-f) and/or slow (-s) implementations')
    print('')
    print('If a double-dash appears, it signals the end of the options and\n' +
        'the beginning of a comment. This comment is optional and will appear\n' +
        'in the log files when in benchmark mode.')
    print('')
    print('Output from the LDA executable is colored in green.')
    print('')
    print('Generated reference data is available in the folder `%s`' % REFERENCE_FOLDER)
    print('Benchmark timings are available in the folder `%s`' % TIMING_FOLDER)

    sys.exit()

def list_from_range(r):
    if ',' in r:
        parts = r.split(',')
        if len(parts) != 3:
            raise ValueError('Range should be of the form start,end,step')

        # +1 on the end because otherwise it's not very intuitive
        return list(range(int(parts[0]), int(parts[1]) + 1, int(parts[2])))
    else:
        return [int(r)]

if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] not in {'gen', 'test', 'bench'}:
        usage_and_quit()

    mode = sys.argv[1]
    try:
        ddash = sys.argv.index('--')
        options = sys.argv[1:, ddash]
        comment = ' '.join(sys.argv[ddash + 1:])
    except:
        options = sys.argv[1:]
        comment = ''

    opts, args = getopt.gnu_getopt(options, "fs",
        ["num-topics=",
        "num-docs="])

    do_fast = False
    do_slow = False

    ks = [50]
    ns = [2246] # Maximal amount of documents

    for o, a in opts:
        if o == '--num-topics':
            ks = list_from_range(a)
        elif o == '--num-docs':
            ns = list_from_range(a)
        elif o == '-f':
            do_fast = True
        elif o == '-s':
            do_slow = True

    if mode == 'gen':
        fn = generate
    elif mode == 'test':
        fn = test
    elif mode == 'bench':
        if not do_fast and not do_slow:
            raise ValueError('When benchmarking, specify at least one of -s (slow), -f (fast)')
        fn = lambda x, y: bench(x, y, do_fast, do_slow)

        RUN_NAME = time.strftime('%Y-%m-%d_%H-%M-%S')
        os.makedirs(TIMING_FOLDER % RUN_NAME)

    if not exists(REFERENCE_FOLDER):
        os.mkdir(REFERENCE_FOLDER)

    for k in ks:
        for n in ns:
            fn(k, n)
