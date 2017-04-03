#include <stdlib.h>
#include <string.h>

#include "dict.h"
#include "vector.h"

/* A relatively simple implementation of a dictionary, which remembers
 * string <-> index associations. Adding a string to the dictionary associates
 * it with a unique ID. The dictionary supports retrieving the ID of a word, and
 * the word of an ID.
 *
 * The ID -> word association is simply an array of char*s A, where A[i] is the
 * word with ID i.
 *
 * The word -> ID association is maintained by a hash table. The hash table is
 * backed by an array H, where H[i] contains the ID of the word with hash i.
 *
 * Collisions are handled by "open addressing": If a word collides, it takes
 * the next available spot in the table. When looking up values, we check
 * the table sequentially starting from the hash and continuing until we reach
 * the correct value or an empty spot.
 *
 * Once the load factor (proportion of filled slots) goes above a certain
 * threshold, we expand the table to keep lookups fast.
 *
 * The representation of an empty entry in the table is a dict_entry_t
 * with id == 0 and word == 0. */

/* The maximal load factor of dictionaries. Once the proportion of full entries
 * is above this number, inserting a new word in the dictionary will expand the
 * table.
 *
 * With our open addressing, the load factor should be kept quite low to avoid
 * tending towards linear lookup time. */
#define MAX_LOAD_FACTOR 0.3

/* Various primes are used for the default capacities. Once we need a larger
 * capacity, we simply double the previous one. */
static int DEFAULT_CAPS[] = {1009,  2521,   4999,   8581,   13183,
                             20063, 27073,  47207,  60107,  74729,
                             90599, 197117, 300007, 432007, 999959};
#define N_DEFAULT_CAPS 15

/* Jenkins hash */
static unsigned int hash(const char* str) {
  unsigned int hash = 0;
  while (*str) {
    hash += *str++;
    hash += hash << 10;
    hash ^= hash >> 6;
  }

  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}

/* Linear probe for the given word in the dict, starting from 'whence'.
 * If 'last_searched' is not NULL, it will be fed with the last table index
 * that was looked at in the lookup. */
static int lookup(const dict_t* dict, const char* word,
                  unsigned int whence, int* last_searched) {
    int keep_going = 1;
    int res = -1;
    while (keep_going) {
        if (dict->indices_by_word[whence].word == 0) {
            keep_going = 0;
        } else if (strcmp(dict->indices_by_word[whence].word, word) == 0) {
            res = dict->indices_by_word[whence].id;
            keep_going = 0;
        } else {
            // Technically we should have a check to make sure we didn't loop
            // all the way back. However, the table will never be totally full.
            whence = (whence + 1) % dict->indices_cap;
        }
    }

    // This will be used by the insert function
    if (last_searched) {
        *last_searched = whence;
    }

    return res;
}

/* Insert the given entry at the first free position starting from index 'where'. */
static void insert(dict_entry_t* entries, dict_entry_t what, int where, int len) {
    while (1) {
        if (entries[where].word == 0) {
            entries[where] = what;
            return;
        }
        where = (where + 1) % len;
    }
}

/* Extend the hash table to lower the load factor.
 * A new backing array is allocated, and all entries are rehashed and
 * reinserted at their new position in the array. */
static void expand(dict_t* dict) {
    int i;
    int old_cap = dict->indices_cap;

    /* If we can, use the next available default capacity.
     * If not, just double the current capacity. */
    dict->default_cap_index++;
    if (dict->default_cap_index < N_DEFAULT_CAPS) {
        dict->indices_cap = DEFAULT_CAPS[dict->default_cap_index];
    } else {
        dict->indices_cap = dict->indices_cap * 2;
    }

    // Rehash all the old entries and put them in their new position.
    dict_entry_t* new_hashes = calloc(dict->indices_cap, sizeof(dict_entry_t));
    for (i = 0 ; i < old_cap ; i++) {
        if (dict->indices_by_word[i].word) {
            int new_hash = hash(dict->indices_by_word[i].word) % dict->indices_cap;
            insert(new_hashes, dict->indices_by_word[i], new_hash, dict->indices_cap);
        }
    }

    // Swap out the old hash table
    free(dict->indices_by_word);
    dict->indices_by_word = new_hashes;
}





int index_from_word(const dict_t* dict, const char* word) {
    if (!word) {
        return -1;
    }

    unsigned int h = hash(word) % dict->indices_cap;
    return lookup(dict, word, h, NULL);
}

char* word_from_index(const dict_t* dict, const int index) {
    if (index < 0 || index >= vec_len(dict->words_by_index_v)) {
        return NULL;
    }

    return dict->words_by_index_v[index];
}

int insert_word(dict_t* dict, const char* word) {

    // If the load factor is too high, expand the table.
    float load = vec_len(dict->words_by_index_v) / (float) dict->indices_cap;
    if (load > MAX_LOAD_FACTOR)
        expand(dict);


    // If the word is already contained, we don't do anything.
    unsigned int h = hash(word) % dict->indices_cap;
    int first_slot; // Will contain the first empty spot if the word isn't there.
    int search_result = lookup(dict, word, h, &first_slot);
    if (search_result != -1)
        return search_result;

    // Generate a new index, and insert the word in the indices by word
    int new_id = vec_len(dict->words_by_index_v);
    char* local_word = malloc(strlen(word));
    strcpy(local_word, word); // Keep a local copy; the other might be freed later.
    dict_entry_t new_entry = {new_id, local_word};
    dict->indices_by_word[first_slot] = new_entry;

    // Insert the word in the vector
    vec_append(dict->words_by_index_v, &local_word, sizeof(char*));

    return new_id;
}

dict_t* dict_alloc() {
    dict_t* dict = malloc(sizeof(dict_t));
    dict->indices_by_word = calloc(DEFAULT_CAPS[0], sizeof(dict_entry_t));
    dict->indices_cap = DEFAULT_CAPS[0];
    dict->words_by_index_v = vec_alloc(DEFAULT_CAPS[0], sizeof(char*));
    dict->default_cap_index = 0;

    return dict;
}

int dict_len(dict_t* dict) {
    return vec_len(dict->words_by_index_v);
}

void dict_free(dict_t* dict) {
    int i;
    for (i = 0 ; i < vec_len(dict->words_by_index_v) ; i++) {
        free(dict->words_by_index_v[i]);
    }

    vec_free(dict->words_by_index_v);
    free(dict->indices_by_word);
    free(dict);
}
