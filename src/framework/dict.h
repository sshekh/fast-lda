#ifndef DICT_H
#define DICT_H

/* A two-way hash-table-like structure, that stores associations between words
 * and their indices.
 */
struct dict_t {
    // Vector of words; the ith element is the word associated with index i.
    char** words_by_index_v;

    /* Table of word indices; the ith element is the word index associated with
     * the word that has hash i. */
    int* indices_by_word;

    /* Size of the 'indices_by_word' table. */
    int indices_cap;
};
typedef struct dict_t dict_t;

/**
 * Obtain the index of a word from this dictionary. If the word is not present,
 * -1 is returned.
 */
int index_from_word(const dict_t* dict, const char* word);

/**
 * Obtain the word associated to a certain index. If index > dict->len, NULL is
 * returned.
 */
char* word_from_index(const dict_t* dict, const int index);

/**
 * Insert a word in the dictionary. If the word is already present, the
 * dictionary is unchanged and the word's index is returned. If not, the word
 * is inserted, and its newly generated index is returned.
 */
int insert_word(dict_t* dict, const char* word);

/**
 * Obtain the number of elements stored in this dictionary.
 */
int dict_len(dict_t* dict);

/**
 * Create an empty dictionary structure. It must be freed with dict_free().
 */
dict_t* dict_alloc();
void dict_free(dict_t*);

#endif