#ifndef LOAD_H
#define LOAD_H

#include "options.h"

int select_makefiles(struct options *options);
int load_makefiles(struct makefile *makefile, const struct options *options,
                   struct string_list **missing, FILE *standard_input);
int cache_standard_input(const struct options *options, FILE **cached);
int prepare_makefiles(struct makefile *makefile, const struct options *options);

#endif /* ! LOAD_H */
