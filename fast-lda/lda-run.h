#ifndef LDA_RUN_H
#define LDA_RUN_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "lda-estimate.h"


#define OFFSET 0;                  // offset for reading data

/*
 * Read the settings from the file.
 * MAYBE TO DO: Nicer way to set the parameters.
 *
 */
void read_settings(char* filename);

corpus* read_data(char* data_filename, int doc_limit);

#endif
