#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vector.h"

/* A vector is simply represented as a pointer to the data. We smuggle the
 * required additional information (current length and total capacity) in the
 * memory just before the data.
 */

// Obtain a pointer to the length of this vector
#define VEC_LEN_PTR(v)      ((int*) ((char*) (v) - 2*sizeof(int)))
// Obtain a pointer to the capacity of this vector
#define VEC_CAP_PTR(v)      (VEC_LEN_PTR(v) + 1)
// Obtain a pointer to the start of user data
#define VEC_USER_DATA(v)    ((void*) ((char*) (v) + 2 * sizeof(int)))

// Starting capacity of freshly allocated vectors (in number of elements)
#define DEFAULT_ALLOC_CAP 4

/* Allocates a generic vector, with a given capacity and elements of given size. */
void* vec_alloc(int cap, size_t elem_size) {

    // Our layout is: [ [len (int)]  [cap (int)]  [data (cap * elem_size)] ]
    // And we return a pointer to here -----------^

    int* container = malloc(2 * sizeof(int) + elem_size * DEFAULT_ALLOC_CAP);
    container[0] = 0;
    container[1] = cap;
    return VEC_USER_DATA(container);
}

int* vec_int_alloc(int cap) {
    return vec_alloc(cap, sizeof(int));
}

int vec_len(void* vec) {
    return *VEC_LEN_PTR(vec);
}

void* vec_append(void* vec, void* val, size_t elem_size) {
    int len = *VEC_LEN_PTR(vec);
    int cap = *VEC_CAP_PTR(vec);

    // Extend the vector if necessary
    if (len == cap) {
        void* new_container = realloc(VEC_LEN_PTR(vec), cap * 2 * elem_size);
        vec = VEC_USER_DATA(new_container);
        *VEC_CAP_PTR(vec) = cap * 2;
    }

    // Copy the new element to the end.
    memcpy(((char*) vec) + len * elem_size, val, elem_size);
    *VEC_LEN_PTR(vec) = len + 1;
    return vec;
}

int* vec_int_append(int* vec, int val) {
    /*int len = *VEC_LEN_PTR(vec);
    int cap = *VEC_CAP_PTR(vec);

    if (len == cap) {
        void* new_container = realloc(VEC_LEN_PTR(vec), cap * 2 * sizeof(int));
        vec = VEC_USER_DATA(new_container);
        *VEC_CAP_PTR(vec) = cap * 2;
    }

    vec[len] = val;
    *VEC_LEN_PTR(vec) = len + 1;*/
    return vec_append(vec, &val, sizeof(int));
}




void vec_free(void* vec) {
    // Seems weird, but the pointer to the length is the pointer to the
    // beginning of the memory region we allocated for this vector.
    free(VEC_LEN_PTR(vec));
}
