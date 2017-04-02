#ifndef VECTOR_H
#define VECTOR_H

/* A small library for vectors, i.e. growable, random-access, contiguous
 * containers.
 *
 * To use, first allocate a vector of the appropriate type with vec_*_alloc.
 * Data can be accessed using regular array syntax, e.g.:
 *
 *    for (i = 0 ; i < vec_len(my_vector) ; i++)
 *      use(my_vector[i]);
 *
 * The vec_append function can be used to add new elements at the end of the
 * vector. Note that this may change the location of the elements in memory;
 * therefore, do not use an old copy of the vector after having appended to it:
 *    int* my_old_vec = vec;
 *    vec_int_append(vec, 4);
 *    use(my_old_vec[0]); // Dangerous!
 *
 * Instead, always assign the result of vec_append to a variable and use that.
 *
 *    vec = vec_int_append(vec, 4);
 *    use(vec[0]); // OK
 *
 * Finally, do not forget to free vectors once you are done using them. Do not
 * use the free() function from stdlib, but rather vec_free().
 *
 * A specialization for integer vectors is provided. More might be added later.
 */

/* Allocate a vector containing either integers or pointers. cap refers to the
 * initial capacity, i.e. how many elements can be added to the vector before
 * it must be resized. A fresh vector contains no elements and thus has length 0.
 */
void* vec_alloc(int cap, size_t elem_size);
int* vec_int_alloc(int cap);

/* Obtain the length of this vector (i.e. how many elements it contains). */
int vec_len(void* vec);

/* Append a value to this vector.
 * 'vec' is a pointer to the vector.
 * 'val' is a pointer to the value to be appended.
 * 'elem_size' is the size of elements contained in this vector.
 * Returns the current location of the vector. If the vector needed to be
 * resized, this value may be different from 'vec'.
 */
void* vec_append(void* vec, void* val, size_t elem_size);

/* Appennd specialization for int vectors.
 * 'val' is the int to append (not a pointer like the generic vec_append)
 */
int* vec_int_append(int* vec, int val);

void vec_free(void* vec);

#endif