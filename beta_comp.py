#! /usr/bin/python

# usage: python beta_comp.py <beta file>
#
# <beta file> is output from the lda-c code

import sys
import numpy as np

def IsApproximatelyEqual(x, y, epsilon):
    """Returns True iff y is within relative or absolute 'epsilon' of x.
    By default, 'epsilon' is 1e-6.
    """
    # Check absolute precision.
    if -epsilon <= x - y <= epsilon:
        return True

    # Since these are log values, difference doesn't matter if both values
    # are < 100
    if x < -100 and y < -100:
        return True

    return False

def compare(f_ref, f_opt, top_x = 20, epsilon = 1e-1):
    topics_ref = f_ref.readlines()
    topics_opt = f_opt.readlines()
    if len(topics_ref) != len(topics_opt):
        print("Number of topics not equal!!, file 1 has %d, file 2 %d\n" % (len(topics_ref), len(topics_opt)))
        return False
    for i in range(len(topics_ref)):
        one_topic_ref = [float(x) for x in topics_ref[i].split()]
        one_topic_opt = [float(x) for x in topics_opt[i].split()]
        if len(one_topic_ref) != len(one_topic_opt):
            print("Number of words in topic %d not equal!!, file 1 has %d, file 2 %d\n" % (i, len(one_topic_ref), len(one_topic_opt)))
            return False
        else:
            # First reverse to descending order, then pick TOP X topics from the reference file, only compare on those.
            ind_ref = np.argsort(one_topic_ref)[::-1][:top_x]
            for j in ind_ref:
                if not IsApproximatelyEqual(one_topic_ref[j], one_topic_opt[j], epsilon):
                    print("\nVALIDATION FAILED")
                    print("Validation threshold: %f" % epsilon)
                    print("Word %d in topic %d: %f %f" % (j, i, one_topic_ref[j], one_topic_opt[j]))
                    print("\nAborting validation...\n")
                    return False

    return True



if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('usage: python beta_comp.py <beta-file1> <beta-file2>\n')
        sys.exit(1)

    beta_file1 = sys.argv[1]
    beta_file2 = sys.argv[2]
    compare(open(beta_file1, "r"), open(beta_file2, "r"))
