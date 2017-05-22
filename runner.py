import sys
import os
import getopt
import subprocess
import time
import platform

from colorama import init, Fore, Style
from git import Repo

import beta_comp

REFERENCE_FOLDER = 'reference'
REFERENCE_DATA = REFERENCE_FOLDER + '/ref-%d-%d-%s.beta'
LDA_EXE_LOG = './%s-lda/logfiles'
LDA_OUT_BETA = LDA_EXE_LOG + '/final.beta'
LDA_RESULTS = './results'
LDA_OUT_TIMING = LDA_RESULTS + '/timings.csv'

ALWAYS_GENERATE_REF = False
ALWAYS_USE_REF = False

TIMING_FOLDER = 'timings/%s/'
TIMING_FILENAME = '%s_timings_%d_%d.csv'

RUN_NAME = None

# Location of bat files that must be run to set up the env for icl (win only)
# Normally this should be an environment variable or something, but only one
# team member uses windows.
VC_VARS = 'C:\\tools\\vs_comm\\VC\\vcvarsall.bat'
ICL_VARS = 'C:\\tools\\icc\\compilers_and_libraries_2017\\windows\\bin\\iclvars.bat intel64'


# Command-line option: always generate missing refs and reuse existing ones.
NO_PROMPT = False

def run_cmd(cmd, quit_on_fail=True):
    print(Fore.LIGHTBLUE_EX)
    print(cmd)
    print(Style.RESET_ALL)

    print(Fore.LIGHTGREEN_EX)
    print(' ========== ')

    if type(cmd) is type([]): # List of arguments: use subprocess
        ret = subprocess.call(cmd)
    elif type(cmd) is type(''): # Use 'system'
        ret = os.system(cmd)

    print(' ========== ')
    print(Style.RESET_ALL)

    if quit_on_fail and ret != 0:
        sys.exit(ret)


def make_lda_params(which, k, n):
    return ['./%s-lda/lda' % which,           # Executable location
            'est',                              # Execution mode (always est)
            str(n),                             # Number of documents
            '1',                                # Initial estimate for alpha
            str(k),                             # Number of topics
            'master-settings.txt',              # Settings location
            './%s-lda/ap/ap.dat' % which,       # Documents location
            'random',                           # Initialization method (only random)
            LDA_EXE_LOG % which]                # Output directory

def exists(path):
    try:
        os.stat(path)
        return True
    except FileNotFoundError:
        return False

def prompt_yna(always_flag):
    if NO_PROMPT:
        print('y (automatic no-prompt mode)')
        return True

    while True:
        inp = input('(y)es, (n)o, yes to (a)ll\n')
        if inp[0] in {'y', 'n', 'a'}:
            always_flag = inp[0] == 'a'
            return inp[0] != 'n'


def generate(k, n, dbl, overwrite=False):
    # Run the slow LDA, collect the final beta file and move it in the reference.
    print('Generating k=%d n=%d' % (k, n))

    dst = REFERENCE_DATA % (k, n, dbl)

    if exists(dst):
        if overwrite:
            os.remove(dst)
        else:
            print('Already exists. Skipping...')
            return

    run_cmd(make_lda_params('slow', k, n))
    os.renames(LDA_OUT_BETA % 'slow', dst)


def test(k, n, dbl=""):
    global ALWAYS_GENERATE_REF
    global ALWAYS_USE_REF
    # Run the fast LDA and compare the resulting final beta against the reference.
    print('Testing fast against reference k=%d n=%d' % (k, n))
    which = REFERENCE_DATA % (k, n, dbl)

    if not exists(which):
        print('Reference file %s not found... ' % which)
        if not ALWAYS_GENERATE_REF:
            print('Do you want to generate it?')
            if not prompt_yna(ALWAYS_GENERATE_REF):
                print('Aborting')
                sys.exit(0)

        generate(k, n, dbl)
    else:
        print('Reference file %s is available... ' % which)
        if not ALWAYS_USE_REF:
            print('Do you want to use the available reference file?')
            if not prompt_yna(ALWAYS_USE_REF):
                generate(k, n, dbl, overwrite=True)


    run_cmd(make_lda_params('fast', k, n))

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
        print('Test %d %d ran successfully\n' % (k, n))

def bench(k, n, which):

    print('Benchmarking %s k=%d n=%d' % (str(which), k, n))
    for lda in which:
        lda_part = ' '.join(make_lda_params(lda, k, n))
        perf_part = ('perf stat -e instructions,cycles,cache-references,' +
                    'cache-misses,LLC-loads,LLC-load-misses,LLC-stores,' +
                    'LLC-store-misses,dTLB-loads,dTLB-load-misses,dTLB-stores,' +
                    'dTLB-store-misses')

        target = TIMING_FOLDER % RUN_NAME + ('/perf_%s_%d_%d.txt' % (lda, k, n))

        if platform.system() == 'Linux':
            run_cmd(perf_part + ' ' + lda_part + ' 2> ' + target)
        else:
            with open(target,'a') as tar:
                tar.write("\n'perf stat' could not be executed as this command is only available on Linux!\n")

        timing_out = (TIMING_FOLDER % RUN_NAME) + (TIMING_FILENAME % (lda, k, n))
        os.rename(LDA_OUT_TIMING, timing_out)

def record_vitals(comment, options, cmdline):
    os.makedirs(TIMING_FOLDER % RUN_NAME)

    with open((TIMING_FOLDER % RUN_NAME) + '/info.txt', 'w') as minilog:

        # Header
        minilog.write('Bench run %s\n' % RUN_NAME)
        minilog.write('Ran with the command-line `' + ' '.join(cmdline) + '`\n')
        minilog.write('Comment: "' + comment + '"\n')
        minilog.write('===============================\n\n')

        minilog.write('OPTIONS\n')
        for k, v in options.items():
            minilog.write(str(k) + '=' + str(v) + '\n')
        minilog.write('END OPTIONS\n\n')

        # git information
        repo = Repo('.')
        try:
            branch = repo.active_branch.name
        except:
            branch = "[Detached HEAD]"

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
        with open('./master-settings.txt') as setts:
            settings = setts.readlines()
            minilog.writelines(settings)

        minilog.write('\n')

def usage_and_quit():
    print('Fast runner')
    print('')
    print('usage: %s [ gen | test | bench ] options...' % sys.argv[0])
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
    print('The commands run by this script (make, lda, ...) appear in light\n' +
        'blue. The outputs of those commands appear in light green.')
    print('')
    print('Generated reference data is available in the folder `%s`' % REFERENCE_FOLDER)
    print('Benchmark timings are available in the folder `%s`' % TIMING_FOLDER)
    print('')
    print('Other options:')
    print('')
    print('\t-m: Do not run make before running a task. Ignored in bench mode.')
    print('\t-s: Silence lda output (always enabled in bench mode).')
    print('\t-a: No-prompt mode (always generate missing refs / reuse existing).')
    print('\t-r: Reduce precision to floats instead of doubles in the fast.')
    print('\t-g: Compile with gcc instead of icc.')

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

def construct_make_command(which, defines, use_icc):
    # Clean part
    clean_command = 'cd %s-lda && make clean && ' % which
    compile_command = ''

    # Select the compiler to use for the fast
    if use_icc:
        if os.name == 'nt': # ICL on windows
            compiler = ' CC=icl'
            # Run some stuff to set up the environment
            compile_command = VC_VARS + ' && ' + ICL_VARS + ' && '
        elif os.name == 'posix': # ICC on unixy systems
            compiler = ' CC=icc'
    else:
        compiler = ' CC=gcc' #gcc by default but better be explicit

    # Construct the actual make command
    compile_command += ' make'
    xflags = xflags_from_list(defines)
    if xflags != '':
        compile_command += (' XCFLAGS="%s"' % xflags)

    return clean_command + compile_command + compiler


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

    opts, args = getopt.gnu_getopt(options, "fsmrsga",
        ["num-topics=",
        "num-docs=",
        "n=",
        "k="])

    do_fast = False
    do_slow = False
    do_make = True
    use_doubles = True
    silence_output = mode == 'bench'
    use_icc = True


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
        elif o == '-r':
            use_doubles = False
        elif o == '-s':
            silence_output = True
        elif o == '-a':
            NO_PROMPT = True
        elif o == '-g':
            use_icc = False

    RUN_NAME = time.strftime('%Y-%m-%d_%H-%M-%S')

    if do_make:

        # Check which defines we need to add
        defines_fast = [] 
        defines_slow = [] # ALWAYS compile with doubles in the slow.
        if not use_doubles:
            defines_fast.append('FLOAT')
        if silence_output:
            defines_fast.append('IGNORE_PRINTF')
            defines_slow.append('IGNORE_PRINTF')

        # Actually make the programs
        print('Preparing the fast...')
        # Use specified compiler for the fast (gcc or icc)
        run_cmd(construct_make_command('fast', defines_fast, use_icc))

        print('Preparing the slow...')
        run_cmd(construct_make_command('slow', defines_slow, use_icc))

    # Use different names for the reference files depending on whether we're
    # using floats or doubles.
    if use_doubles:
        ref_type_name = 'dbl'
    else:
        ref_type_name = 'flt'

    # Create folders as required
    if not exists(REFERENCE_FOLDER):
        os.mkdir(REFERENCE_FOLDER)
    if not exists(LDA_EXE_LOG % 'fast'):
        os.mkdir(LDA_EXE_LOG % 'fast')
    if not exists(LDA_EXE_LOG % 'slow'):
        os.mkdir(LDA_EXE_LOG % 'slow')
    if not exists(LDA_RESULTS):
        os.mkdir(LDA_RESULTS)

    config_script= "mklvars.sh"

    if os.name == 'posix':
        os.system("source " + config_script + " intel64")
    else:
        print("Insert Windows shit to execute the Intel MKL loading script")
        sys.exit()

    if mode == 'gen':
        fn = lambda x, y: generate(x, y, ref_type_name)
    elif mode == 'test':
        fn = lambda x, y: test(x, y, 'dbl')
    elif mode == 'bench':
        if not do_fast and not do_slow:
            raise ValueError('When benchmarking, specify at least one of -s (slow), -f (fast)')

        # Which versions of lda to run (in bench or perf mode)
        which = []
        if do_fast:
            which.append('fast')
        if do_slow:
            which.append('slow')

        params = {
            'use_doubles': use_doubles,
            'use_icc': use_icc
        }

        record_vitals(comment, params, options)

        fn = lambda x, y: bench(x, y, which)

    for k in ks:
        for n in ns:
            fn(k, n)

