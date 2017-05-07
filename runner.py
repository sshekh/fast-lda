'''
We would like to:
    - Generate test data from the slow
    - Test the fast against the slow
    - Benchmark the fast (or the slow)


'''

import sys
import getopt

REFERENCE_DATA = 'reference'

def generate(k, n):
    # Run the slow LDA
    # Collect the file final.beta
    # Move it into the reference folder, renaming it as k-n.beta or w/e
    print('Generating k=%d n=%d' % (k, n))

def test(k, n):
    # Run the fast LDA
    # Collect the appropriate file final.beta
    # Call the comparison script with the appropriate files
    print('Testing fast against reference k=%d n=%d' % (k, n))

def bench(k, n, fast, slow):
    if fast and slow:
        which = 'fast and slow'
    elif fast:
        which = 'fast'
    elif slow:
        which = 'slow'

    print('Benchmarking %s k=%d n=%d' % (which, k, n))

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

    for k in ks:
        for n in ns:
            fn(k, n)

