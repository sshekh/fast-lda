import sys
import os
import getopt
import subprocess
import time

from colorama import init, Fore, Style
from git import Repo

import beta_comp

REFERENCE_FOLDER = 'reference'
REFERENCE_DATA = REFERENCE_FOLDER + '/ref-%d-%d-%s.beta'
LDA_EXE_LOG = './%s-lda/logfiles'
LDA_OUT_BETA = LDA_EXE_LOG + '/final.beta'
LDA_OUT_TIMING = './results/timings.csv'

ALWAYS_GENERATE_REF = False
ALWAYS_USE_REF = False

TIMING_FOLDER = 'timings/%s/'
TIMING_FILENAME = '%s_timings_%d_%d.csv'

RUN_NAME = None

def quit_on_fail(i):
    if i != 0:
        print(Style.RESET_ALL)
        sys.exit()

def run_lda(which, k, n):
    params = ['./%s-lda/lda' % which,           # Executable location
            'est',                              # Execution mode (always est)
            str(n),                             # Number of documents
            '1',                                # Initial estimate for alpha
            str(k),                             # Number of topics
            'master-settings.txt',              # Settings location
            './%s-lda/ap/ap.dat' % which,       # Documents location
            'random',                           # Initialization method (only random)
            LDA_EXE_LOG % which]                # Output directory

    print(Fore.LIGHTBLUE_EX)
    print(params)
    print(Style.RESET_ALL)
    print(Fore.LIGHTGREEN_EX)
    print(' ========== ')
    quit_on_fail(subprocess.call(params))
    print(' ========== ')
    print(Style.RESET_ALL)


def exists(path):
    try:
        os.stat(path)
        return True
    except FileNotFoundError:
        return False

def prompt_yna(always_flag):
    while True:
        inp = input('(y)es, (n)o, yes to (a)ll\n')
        if inp[0] in {'y', 'n', 'a'}:
            always_flag = inp[0] == 'a'
            return inp[0] != 'n'


def generate(k, n, dbl, overwrite=False):
    # Run the slow LDA, collect the final beta file and move it in the reference.
    print('Generating k=%d n=%d' % (k, n))

    dst = REFERENCE_DATA % (k, n, dbl)

    if exists(dst) and overwrite == False:
        print('Already exists. Skipping...')
        return

    run_lda('slow', k, n)
    os.renames(LDA_OUT_BETA % 'slow', dst)


def test(k, n, dbl):
    global ALWAYS_GENERATE_REF
    global ALWAYS_USE_REF
    # Run the fast LDA and compare the resulting final beta against the reference.
    print('Testing fast against reference k=%d n=%d' % (k, n))
    which = REFERENCE_DATA % (k, n, dbl)

    if not exists(which):
        print('Reference file %s not found... ' % which, end='\n')
        if not ALWAYS_GENERATE_REF:
            print('Do you want to generate it?')
            if not prompt_yna(ALWAYS_GENERATE_REF):
                print('Aborting')
                sys.exit(0)

        generate(k, n, dbl)
    else:
        print('Reference file %s is available... ' % which, end='\n')
        if not ALWAYS_USE_REF:
            print('Do you want to use the available reference file?')
            if not prompt_yna(ALWAYS_USE_REF):
                generate(k, n, dbl, overwrite=True)


    run_lda('fast', k, n)

    reference = open(which, 'r')
    created = open(LDA_OUT_BETA % 'fast', 'r')

    print('Comparing against the reference...')
    good = beta_comp.compare(reference, created)

    reference.close()
    created.close()

    if not good:
        # The comparator script has printed the relevant info, just quit.
        sys.exit(1)
    else:
        print('Test %d %d ran successfully' % (k, n))

def bench(k, n, which):

    print('Benchmarking %s k=%d n=%d' % (str(which), k, n))
    for lda in which:
        run_lda(lda, k, n)
        timing_out = (TIMING_FOLDER % RUN_NAME) + (TIMING_FILENAME % (lda, k, n))
        os.rename(LDA_OUT_TIMING, timing_out)

def record_vitals(comment):
    os.makedirs(TIMING_FOLDER % RUN_NAME)

    with open((TIMING_FOLDER % RUN_NAME) + '/info.txt', 'w') as minilog:

        # Header
        minilog.write('Bench run %s\n' % RUN_NAME)
        minilog.write('"' + comment + '"\n')
        minilog.write('===============================\n\n')


        # git information
        repo = Repo('.')
        branch = repo.active_branch.name
        commit = repo.commit().hexsha[:8] # commit by itself returns the last commit

        minilog.write('Latest commit at time of writing:\n')
        minilog.write('%s (on branch %s)\n\n' % (commit, branch))

        if len(repo.untracked_files) != 0:
            minilog.write('Additionally, the repo contained the following untracked files:\n')
            for f in repo.untracked_files:
                minilog.write('\t' + f + '\n')
            minilog.write('\n')

        minilog.write('The diff, if any, follows:\n\n')
        minilog.write(repo.git.diff())
        minilog.write('\n\n===============================\n\n')

        # Settings
        minilog.write('Settings file:\n')
        with open('../master-settings.txt') as setts:
            settings = setts.readlines()
            minilog.writelines(settings)

        minilog.write('\n')

def usage_and_quit():
    print('Fast runner')
    print('')
    print('usage: %s [ gen | test | bench ] options... [-- comment]' % sys.argv[0])
    print('')
    print('Options: ')
    print('The following two options recieve as argument either one number,\n' +
        'or three separated by a comma (e.g. `--num-docs=50,100,10`). These\n' +
        'three numbers specify the start, end and step values. If a single\n' +
        'number is specified, only that value will be used.' )
    print('\t--num-topics / --k (default: 50)')
    print('\t--num-docs / --n (default: all documents)')
    print('')
    print('Modes:')
    print('\tgen:   Generate test data from the slow implementation')
    print('\ttest:  Obtain results from the fast implementation, and compare ')
    print('\t       against reference output from the slow implementation.')
    print('\tbench: Check speed of fast (-f) and/or slow (-s) implementations')
    print('')
    print('If a double-dash appears, it signals the end of the options and\n' +
        'the beginning of a comment. This comment is mandatory in benchnamrk\n' +
         'mode and appears in the log files.')
    print('')
    print('Output from the LDA executable and make is colored in green.')
    print('')
    print('Generated reference data is available in the folder `%s`' % REFERENCE_FOLDER)
    print('Benchmark timings are available in the folder `%s`' % TIMING_FOLDER)
    print('')
    print('Other options:')
    print('')
    print('\t-m: Do not run make before running a task. Ignored in bench mode.')
    print('\t-v: When benchmarking, also validate before.')
    print('\t-d: Use doubles instead of floats.')
    print('\t-s: Silence output (always enabled in bench mode).')

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

def xflags_from_list(defines):
    with_d = ['-D' + x for x in defines]
    return ' '.join(with_d)

if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] not in {'gen', 'test', 'bench'}:
        usage_and_quit()

    mode = sys.argv[1]
    try:
        ddash = sys.argv.index('--')
        options = sys.argv[1:ddash]
        comment = ' '.join(sys.argv[ddash + 1:])
    except ValueError:
        options = sys.argv[1:]
        comment = ''
        if mode == 'bench':
            raise ValueError('When benchmarking, please include a comment.')

    opts, args = getopt.gnu_getopt(options, "fsmvds",
        ["num-topics=",
        "num-docs=",
        "n=",
        "k="])

    do_fast = False
    do_slow = False
    do_make = True
    validate_when_benching = False
    use_doubles = False
    silence_output = mode == 'bench'

    ks = [50]
    ns = [2246] # Maximal amount of documents

    for o, a in opts:
        if o in {'--num-topics', '--k'}:
            ks = list_from_range(a)
        elif o in {'--num-docs', '--n'}:
            ns = list_from_range(a)
        elif o == '-f':
            do_fast = True
        elif o == '-s':
            do_slow = True
        elif o == '-m' and mode != 'bench': # ALWAYS make in bench mode
            do_make = False
        elif o == '-v':
            validate_when_benching = True
        elif o == '-d':
            use_doubles = True
        elif o == '-s':
            silence_output = True


    if do_make:

        # Check which defines we need to add
        defines = []
        if use_doubles:
            defines.append('DOUBLE')
        if silence_output:
            defines.append('IGNORE_PRINTF')

        # Construct the make command
        xflags = xflags_from_list(defines)
        final_command = 'make'
        if xflags != '':
            final_command += (' XCFLAGS="%s"' % xflags)

        # Actually make the programs
        print('Perparing the fast...')
        print(Fore.LIGHTGREEN_EX)
        quit_on_fail(os.system('cd fast-lda && make clean && ' + final_command))
        print(Style.RESET_ALL)

        print('Perparing the slow...')
        print(Fore.LIGHTGREEN_EX)
        quit_on_fail(os.system('cd slow-lda && make clean && ' + final_command))
        print(Style.RESET_ALL)

    # Use different names for the reference files depending on whether we're
    # using floats or doubles.
    if use_doubles:
        ref_type_name = 'dbl'
    else:
        ref_type_name = 'flt'

    if mode == 'gen':
        fn = lambda x, y: generate(x, y, ref_type_name)
    elif mode == 'test':
        fn = lambda x, y: test(x, y, ref_type_name)
    elif mode == 'bench':

        which = []

        if do_fast:
            which.append('fast')
        if do_slow:
            which.append('slow')

        fn = lambda x, y: bench(x, y, which)

        if validate_when_benching:
            print('First validating on a small input...')
            test(10, 100)

        if not do_fast and not do_slow:
            raise ValueError('When benchmarking, specify at least one of -s (slow), -f (fast)')

        RUN_NAME = time.strftime('%Y-%m-%d_%H-%M-%S')

        record_vitals(comment)

    if not exists(REFERENCE_FOLDER):
        os.mkdir(REFERENCE_FOLDER)

    for k in ks:
        for n in ns:
            fn(k, n)

