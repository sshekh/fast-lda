#! /usr/bin/python

# usage: python beta_comp.py <beta file>
#
# <beta file> is output from the lda-c code

import sys

def IsApproximatelyEqual(x, y, epsilon = 1e-6):
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

def compare(f1, f2):
    topics1 = f1.readlines()
    topics2 = f2.readlines()
    if len(topics1) != len(topics2):
        print("Number of topics not equal!!, file 1 has %d, file 2 %d\n" % (len(topics1), len(topics2)))
        return False
    for i in range(len(topics1)):
        topic1 = [float(x) for x in topics1[i].split()]
        topic2 = [float(x) for x in topics2[i].split()]
        if len(topic1) != len(topic2):
            print("Number of words in topic %d not equal!!, file 1 has %d, file 2 %d\n" % (i, len(topic1), len(topic2)))
            return False
        else:
            for j in range(len(topic1)):
                if not IsApproximatelyEqual(topic1[j], topic2[j]):
                    print("Word %d %f %f" % (j, topic1[j], topic2[j]))
                    print("Aborting topic\n")
                    return False

    return True



if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('usage: python beta_comp.py <beta-file1> <beta-file2>\n')
        sys.exit(1)

    beta_file1 = sys.argv[1]
    beta_file2 = sys.argv[2]
    compare(open(beta_file1, "r"), open(beta_file2, "r"))
