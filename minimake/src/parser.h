#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "model.h"

/* Append definitions; return zero on success and two on an error. */
int parse_makefile(struct makefile *makefile, FILE *input,
                   const char *filename);

#endif /* ! PARSER_H */
