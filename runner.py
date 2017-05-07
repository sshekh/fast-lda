'''
We would like to:
    - Generate test data from the slow
    - Test the fast against the slow
    - Benchmark the fast (or the slow)


'''

import sys

REFERENCE_DATA = 'reference'

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
        'in the log files.')

    sys.exit()


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

