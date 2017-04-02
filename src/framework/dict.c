#include <stdlib.h>

#include "dict.h"
#include "vector.h"

/* The maximal load factor of dictionaries. Once the proportion of full entries
 * is above this number, inserting a new word in the dictionary will expand the
 * table.*/
#define MAX_LOAD_FACTOR 0.3

/* The starting capacity of dictionaries. */
#define DEFAULT_CAP 307

int index_from_word(const dict_t* dict, const char* word) {
    // TODO
    // Hash the word and look up the index in dict->indices_by_word[hash]
    // Need a strategy for collisions.
    return -1;
}

char* word_from_index(const dict_t* dict, const int index) {
    return dict->words_by_index_v[index];
}

int insert_word(dict_t* dict, const char* word) {
    // TODO
    // Check the load factor; if we are above max, allocate a new
    // indices_by_word and rehash everything. Then:

    // Hash the word
    // Generate a new index
    // dict->indices_by_word[hash] = new_index
    // (if there are no collisions)

    return -1;
}

dict_t* dict_alloc() {
    dict_t* dict = malloc(sizeof(dict_t));
    dict->indices_by_word = malloc(sizeof(int) * DEFAULT_CAP);
    dict->indices_cap = DEFAULT_CAP;
    dict->words_by_index_v = vec_alloc(DEFAULT_CAP, sizeof(char*));

    return dict;
}

int dict_len(dict_t* dict) {
    return vec_len(dict->words_by_index_v);
}

void dict_free(dict_t* dict) {
    free(dict->indices_by_word);
    vec_free(dict->words_by_index_v);
    free(dict);
}
