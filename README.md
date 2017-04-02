### Structure

The code is found in the 'src' folder.
  * 'src/framework' contains some generic functionality that we will need in
    various places. Currently, it has an implementation for vectors (the
    container, not the mathematical object) and an implementation for
    dictionaries.
  * 'src/topics' contains everything related to the project itself. It contains
    code to deal with documents (loading, parsing, arranging in a corpus) as
    well as the LDA code.
  * 'src/main.c' is the entry point.

Most of the functionality is currently NOT written, but it's mostly down to
filling in stubs, the general structure is there.

Documentation is mainly found in the headers.

To build, run 'make debug' or 'make release'. Specifying just 'make' will build
both debug and release versions, which is probably not what you want.
