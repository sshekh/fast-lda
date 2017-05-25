import os
import nltk
from unidecode import unidecode
from collections import Counter
import sys

def filter_sentence(s):
    words = nltk.tokenize.word_tokenize(s)
    return [unidecode(w.lower()) for w in words if w.isalpha()]


def all_words(filename):
    with open(filename, encoding='utf-8') as f:
        splitted = [filter_sentence(x) for x in f.readlines()]
        return [i for sub in splitted for i in sub]

def create_vocab(europath):
    words = set()

    for fname in os.listdir(europath):
        words |= set(all_words(europath + '/' + fname))

    id_to_word = list(words)
    word_to_id = dict([(w, i) for i, w in enumerate(id_to_word)])

    return (id_to_word, word_to_id)

def create_doc(filename, word_to_id):
    words = all_words(filename)
    counts = Counter(words)

    id_counts = [(word_to_id[word], count) for word, count in counts.items()]

    return (len(id_counts), id_counts)

def doc_to_line(doc):
    return str(doc[0]) + ' ' + ' '.join(['%d:%d' % (w, c) for w, c in doc[1]]) + '\n'

def write_all(vocab, docs):
    with open('europarl_fi.vcb', 'w', encoding='ascii') as f:
        f.writelines([w + '\n' for w in vocab])

    with open('europarl_fi.dat', 'w', encoding='ascii') as f:
        for doc in docs:
            f.write(doc_to_line(doc))

def build(europath):

    id_to_word, word_to_id = create_vocab(europath)

    docs = []

    for fname in os.listdir(europath):
        docs.append(create_doc(europath + '/' + fname, word_to_id))

    write_all(id_to_word, docs)

if __name__ == '__main__':
    build(sys.argv[1])